// (C) 2018 University of Bristol. See License.txt

#include "Machine.h"

#include "Exceptions/Exceptions.h"

#include <sys/time.h>

#include "Math/Setup.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <pthread.h>
using namespace std;

Machine::Machine(int my_number, Names& playerNames,
    string progname_str, string memtype, int lgp, int lg2, bool direct,
    int opening_sum, bool parallel, bool receive_threads, int max_broadcast)
  : my_number(my_number), N(playerNames), nthreads(0), tn(0), numt(0), usage_unknown(false),
    progname(progname_str), direct(direct), opening_sum(opening_sum), parallel(parallel),
    receive_threads(receive_threads), max_broadcast(max_broadcast)
{
  if (opening_sum < 2)
    this->opening_sum = N.num_players();
  if (max_broadcast < 2)
    this->max_broadcast = N.num_players();

  // Set up the fields
  prep_dir_prefix = get_prep_dir(N.num_players(), lgp, lg2);
  read_setup(prep_dir_prefix);

  char filename[1024];
  int nn;

  sprintf(filename, (prep_dir_prefix + "Player-MAC-Keys-P%d").c_str(), my_number);
  inpf.open(filename);
  if (inpf.fail())
  {
    cerr << "Could not open MAC key file. Perhaps it needs to be generated?\n";
    throw file_error(filename);
  }
  inpf >> nn;
  if (nn!=N.num_players())
    { cerr << "KeyGen was last run with " << nn << " players." << endl;
      cerr << "  - You are running Online with " << N.num_players() << " players." << endl;
      exit(1);
    }

  alphapi.input(inpf,true);
  alpha2i.input(inpf,true);
  cerr << "MAC Key p = " << alphapi << endl;
  cerr << "MAC Key 2 = " << alpha2i << endl;
  inpf.close();


  // Initialize the global memory
  if (memtype.compare("new")==0)
     {sprintf(filename, PREP_DIR "Player-Memory-P%d", my_number);
       ifstream memfile(filename);
       if (memfile.fail()) { throw file_error(filename); }
       Load_Memory(M2,memfile); 
       Load_Memory(Mp,memfile); 
       Load_Memory(Mi,memfile);
       memfile.close();
     }
  else if (memtype.compare("old")==0)
     {
       sprintf(filename, PREP_DIR "Memory-P%d", my_number);
       inpf.open(filename,ios::in | ios::binary);
       if (inpf.fail()) { throw file_error(); }
       inpf >> M2 >> Mp >> Mi;
       inpf.close();
     }
  else if (!(memtype.compare("empty")==0))
     { cerr << "Invalid memory argument" << endl;
       exit(1);
     }

  string fname = "Programs/Schedules/" + progname + ".sch";
  cerr << "Opening file " << fname << endl;
  inpf.open(fname);
  if (inpf.fail()) { throw file_error("Missing '" + fname + "'. Did you compile '" + progname + "'?"); }

  int nprogs;
  inpf >> nthreads;
  inpf >> nprogs;

  // Keep record of used offline data
  pos.set_num_players(N.num_players());

  cerr << "Number of threads I will run in parallel = " << nthreads << endl;
  cerr << "Number of program sequences I need to load = " << nprogs << endl;

  // Load in the programs 
  progs.resize(nprogs,N.num_players());
  string threadname;
  for (int i=0; i<nprogs; i++)
    { inpf >> threadname;
      string filename = "Programs/Bytecode/" + threadname + ".bc";
      cerr << "Loading program " << i << " from " << filename << endl;
      ifstream pinp(filename);
      if (pinp.fail()) { throw file_error(filename); }
      progs[i].parse(pinp);
      pinp.close();
      M2.minimum_size(GF2N, progs[i], threadname);
      Mp.minimum_size(MODP, progs[i], threadname);
      Mi.minimum_size(INT, progs[i], threadname);
    }

  progs[0].print_offline_cost();

  /* Set up the threads */
  tinfo.resize(nthreads);
  threads.resize(nthreads);
  t_mutex.resize(nthreads);
  client_ready.resize(nthreads);
  server_ready.resize(nthreads);
  join_timer.resize(nthreads);

  for (int i=0; i<nthreads; i++)
    { pthread_mutex_init(&t_mutex[i],NULL);
      pthread_cond_init(&client_ready[i],NULL);
      pthread_cond_init(&server_ready[i],NULL);
      tinfo[i].thread_num=i;
      tinfo[i].Nms=&N;
      tinfo[i].alphapi=&alphapi;
      tinfo[i].alpha2i=&alpha2i;
      tinfo[i].prognum=-2;  // Dont do anything until we are ready
      tinfo[i].finished=true;
      tinfo[i].ready=false;
      tinfo[i].machine=this;
      // lock for synchronization
      pthread_mutex_lock(&t_mutex[i]);
      pthread_create(&threads[i],NULL,Main_Func,&tinfo[i]);
    }

  // synchronize with clients before starting timer
  for (int i=0; i<nthreads; i++)
    {
      while (!tinfo[i].ready)
        {
          cerr << "Waiting for thread " << i << " to be ready" << endl;
          pthread_cond_wait(&client_ready[i],&t_mutex[i]);
        }
      pthread_mutex_unlock(&t_mutex[i]);
    }
}

DataPositions Machine::run_tape(int thread_number, int tape_number, int arg, int line_number)
{
  if (thread_number >= (int)tinfo.size())
    throw Processor_Error("invalid thread number: " + to_string(thread_number) + "/" + to_string(tinfo.size()));
  if (tape_number >= (int)progs.size())
    throw Processor_Error("invalid tape number: " + to_string(tape_number) + "/" + to_string(progs.size()));
  pthread_mutex_lock(&t_mutex[thread_number]);
  tinfo[thread_number].prognum=tape_number;
  tinfo[thread_number].arg=arg;
  tinfo[thread_number].pos=pos;
  tinfo[thread_number].finished=false;
  //printf("Send signal to run program %d in thread %d\n",tape_number,thread_number);
  pthread_cond_signal(&server_ready[thread_number]);
  pthread_mutex_unlock(&t_mutex[thread_number]);
  //printf("Running line %d\n",exec);
  if (progs[tape_number].usage_unknown())
    { // only one thread allowed
      if (numt>1)
        { cerr << "Line " << line_number << " has " <<
          numt << " threads but tape " << tape_number <<
          " has unknown offline data usage" << endl;
        throw invalid_program();
        }
      else if (line_number == -1)
        {
          cerr << "Internally called tape " << tape_number <<
              " has unknown offline data usage" << endl;
          throw invalid_program();
        }
      usage_unknown = true;
      return DataPositions(N.num_players());
    }
  else
    {
      // Bits, Triples, Squares, and Inverses skipping
      return progs[tape_number].get_offline_data_used();
    }
}

void Machine::join_tape(int i)
{
  join_timer[i].start();
  pthread_mutex_lock(&t_mutex[i]);
  //printf("Waiting for client to terminate\n");
  if ((tinfo[i].finished)==false)
    { pthread_cond_wait(&client_ready[i],&t_mutex[i]); }
  pthread_mutex_unlock(&t_mutex[i]);
  join_timer[i].stop();
}

void Machine::run()
{
  Timer proc_timer(CLOCK_PROCESS_CPUTIME_ID);
  proc_timer.start();
  timer[0].start();

  bool flag=true;
  usage_unknown=false;
  int exec=0;
  while (flag)
    { inpf >> numt;
      if (numt==0) 
        { flag=false; }
      else
        { for (int i=0; i<numt; i++)
            {
	        // Now load up data
                inpf >> tn;

                // Cope with passing an integer parameter to a tape
                int arg;
                if (inpf.get() == ':')
                  inpf >> arg;
                else
                  arg = 0;

                //cerr << "Run scheduled tape " << tn << " in thread " << i << endl;
                pos.increase(run_tape(i, tn, arg, exec));
            }
          // Make sure all terminate before we continue
          for (int i=0; i<numt; i++)
            { join_tape(i);
            }
         if (usage_unknown)
           { // synchronize files
             pos = tinfo[0].pos;
             usage_unknown = false;
           }
         //printf("Finished running line %d\n",exec);
         exec++;
      }
    }

  char compiler[1000];
  inpf.get();
  inpf.getline(compiler, 1000);
  if (compiler[0] != 0)
    cerr << "Compiler: " << compiler << endl;
  inpf.close();

  finish_timer.start();
  // Tell all C-threads to stop
  for (int i=0; i<nthreads; i++)
    { pthread_mutex_lock(&t_mutex[i]);
	//printf("Send kill signal to client\n");
        tinfo[i].prognum=-1;
        tinfo[i].ready = false;
        pthread_cond_signal(&server_ready[i]);
      pthread_mutex_unlock(&t_mutex[i]);
    }

  cerr << "Waiting for all clients to finish" << endl;
  // Wait until all clients have signed out
  for (int i=0; i<nthreads; i++)
    {
      pthread_mutex_lock(&t_mutex[i]);
      tinfo[i].ready = true;
      pthread_cond_signal(&server_ready[i]);
      pthread_mutex_unlock(&t_mutex[i]);
      pthread_join(threads[i],NULL);
      pthread_mutex_destroy(&t_mutex[i]);
      pthread_cond_destroy(&client_ready[i]);
      pthread_cond_destroy(&server_ready[i]);
    }
  finish_timer.stop();
  
  for (unsigned int i = 0; i < join_timer.size(); i++)
    cerr << "Join timer: " << i << " " << join_timer[i].elapsed() << endl;
  cerr << "Finish timer: " << finish_timer.elapsed() << endl;
  cerr << "Process timer: " << proc_timer.elapsed() << endl;
  print_timers();

  if (opening_sum < N.num_players() && !direct)
    cerr << "Summed at most " << opening_sum << " shares at once with indirect communication" << endl;
  else
    cerr << "Summed all shares at once" << endl;

  if (max_broadcast < N.num_players() && !direct)
    cerr << "Send to at most " << max_broadcast << " parties at once" << endl;
  else
    cerr << "Full broadcast" << endl;

  // Reduce memory size to speed up
  int max_size = 1 << 20;
  if (M2.size_s() > max_size)
    M2.resize_s(max_size);
  if (Mp.size_s() > max_size)
    Mp.resize_s(max_size);

  // Write out the memory to use next time
  char filename[1024];
  sprintf(filename,PREP_DIR "Memory-P%d",my_number);
  ofstream outf(filename,ios::out | ios::binary);
  outf << M2 << Mp << Mi;
  outf.close();

  extern unsigned long long sent_amount, sent_counter;
  cerr << "Data sent = " << sent_amount << " bytes in "
      << sent_counter << " calls,";
  cerr << sent_amount / sent_counter / N.num_players()
      << " bytes per call" << endl;

  for (int dtype = 0; dtype < N_DTYPE; dtype++)
    {
      cerr << "Num " << Data_Files::dtype_names[dtype] << "\t=";
      for (int field_type = 0; field_type < N_DATA_FIELD_TYPE; field_type++)
        cerr << " " << pos.files[field_type][dtype];
      cerr << endl;
   }
  for (int field_type = 0; field_type < N_DATA_FIELD_TYPE; field_type++)
    {
      cerr << "Num " << Data_Files::long_field_names[field_type] << " Inputs\t=";
      for (int i = 0; i < N.num_players(); i++)
        cerr << " " << pos.inputs[i][field_type];
      cerr << endl;
    }

  cerr << "Total cost of program:" << endl;
  pos.print_cost();

#ifndef INSECURE
  Data_Files df(N.my_num(), N.num_players(), prep_dir_prefix);
  df.seekg(pos);
  df.prune();
#endif

  cerr << "End of prog" << endl;
}

void BaseMachine::time()
{
  cout << "Elapsed time: " << timer[0].elapsed() << endl;
}

void BaseMachine::start(int n)
{
  cout << "Starting timer " << n << " at " << timer[n].elapsed()
    << " after " << timer[n].idle() << endl;
  timer[n].start();
}

void BaseMachine::stop(int n)
{
  timer[n].stop();
  cout << "Stopped timer " << n << " at " << timer[n].elapsed() << endl;
}

void BaseMachine::print_timers()
{
  cerr << "Time = " << timer[0].elapsed() << " seconds " << endl;
  timer.erase(0);
  for (map<int,Timer>::iterator it = timer.begin(); it != timer.end(); it++)
    cerr << "Time" << it->first << " = " << it->second.elapsed() << " seconds " << endl;
}
