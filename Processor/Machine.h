// (C) 2018 University of Bristol. See License.txt

/*
 * Machine.h
 *
 */

#ifndef MACHINE_H_
#define MACHINE_H_

#include "Processor/Memory.h"
#include "Processor/Program.h"

#include "Processor/Online-Thread.h"
#include "Processor/Data_Files.h"
#include "Math/gfp.h"

#include "Tools/time-func.h"

#include <vector>
#include <map>
using namespace std;

class BaseMachine
{
protected:
    std::map<int,Timer> timer;
    void print_timers();

public:
    void time();
    void start(int n);
    void stop(int n);
};

class Machine : public BaseMachine
{
  /* The mutex's lock the C-threads and then only release
   * then we an MPC thread is ready to run on the C-thread.
   * Control is passed back to the main loop when the
   * MPC thread releases the mutex
   */

  vector<thread_info> tinfo;
  vector<pthread_t> threads;

  int my_number;
  Names& N;
  gfp  alphapi;
  gf2n alpha2i;

  int nthreads;

  ifstream inpf;

  // Keep record of used offline data
  DataPositions pos;

  int tn,numt;
  bool usage_unknown;

  public:

  vector<pthread_mutex_t> t_mutex;
  vector<pthread_cond_t>  client_ready;
  vector<pthread_cond_t>  server_ready;
  vector<Program>  progs;

  Memory<gf2n> M2;
  Memory<gfp> Mp;
  Memory<Integer> Mi;

  vector<Timer> join_timer;
  Timer finish_timer;
  
  string prep_dir_prefix;
  string progname;

  bool direct;
  int opening_sum;
  bool parallel;
  bool receive_threads;
  int max_broadcast;

  Machine(int my_number, Names& playerNames, string progname,
      string memtype, int lgp, int lg2, bool direct, int opening_sum, bool parallel,
      bool receive_threads, int max_broadcast);

  DataPositions run_tape(int thread_number, int tape_number, int arg, int line_number);
  void join_tape(int thread_number);
  void run();
};

#endif /* MACHINE_H_ */
