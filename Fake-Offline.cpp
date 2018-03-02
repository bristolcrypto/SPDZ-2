// (C) 2018 University of Bristol. See License.txt


#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/Share.h"
#include "Math/Setup.h"
#include "Auth/fake-stuff.h"
#include "Exceptions/Exceptions.h"

#include "Math/Setup.h"
#include "Processor/Data_Files.h"
#include "Tools/mkpath.h"
#include "Tools/ezOptionParser.h"
#include "Tools/benchmarking.h"

#include <sstream>
#include <fstream>
using namespace std;


string prep_data_prefix;

/* N      = Number players
 * ntrip  = Number triples needed
 * str    = "2" or "p"
 */
template<class T>
void make_mult_triples(const T& key,int N,int ntrip,const string& str,bool zero)
{
  PRNG G;
  G.ReSeed();

  ofstream* outf=new ofstream[N];
  T a,b,c;
  vector<Share<T> > Sa(N),Sb(N),Sc(N);
  /* Generate Triples */
  for (int i=0; i<N; i++)
    { stringstream filename;
      filename << prep_data_prefix << "Triples-" << str << "-P" << i;
      cout << "Opening " << filename.str() << endl;
      outf[i].open(filename.str().c_str(),ios::out | ios::binary);
      if (outf[i].fail()) { throw file_error(filename.str().c_str()); }
    }
  for (int i=0; i<ntrip; i++)
    {
      if (!zero)
        a.randomize(G);
      make_share(Sa,a,N,key,G);
      if (!zero)
        b.randomize(G);
      make_share(Sb,b,N,key,G);
      c.mul(a,b);
      make_share(Sc,c,N,key,G);
      for (int j=0; j<N; j++)
        { Sa[j].output(outf[j],false);
          Sb[j].output(outf[j],false);
          Sc[j].output(outf[j],false);
        }
    }
  for (int i=0; i<N; i++)
    { outf[i].close(); }
  delete[] outf;
}

void make_bit_triples(const gf2n& key,int N,int ntrip,Dtype dtype,bool zero)
{
  PRNG G;
  G.ReSeed();

  ofstream* outf=new ofstream[N];
  gf2n a,b,c, one;
  one.assign_one();
  vector<Share<gf2n> > Sa(N),Sb(N),Sc(N);
  /* Generate Triples */
  for (int i=0; i<N; i++)
    { stringstream filename;
      filename << prep_data_prefix << Data_Files::dtype_names[dtype] << "-2-P" << i;
      cout << "Opening " << filename.str() << endl;
      outf[i].open(filename.str().c_str(),ios::out | ios::binary);
      if (outf[i].fail()) { throw file_error(filename.str().c_str()); }
    }
  for (int i=0; i<ntrip; i++)
    {
      if (!zero)
        a.randomize(G);
      a.AND(a, one);
      make_share(Sa,a,N,key,G);
      if (!zero)
        b.randomize(G);
      if (dtype == DATA_BITTRIPLE)
        b.AND(b, one);
      make_share(Sb,b,N,key,G);
      c.mul(a,b);
      make_share(Sc,c,N,key,G);
      for (int j=0; j<N; j++)
        { Sa[j].output(outf[j],false);
          Sb[j].output(outf[j],false);
          Sc[j].output(outf[j],false);
        }
    }
  for (int i=0; i<N; i++)
    { outf[i].close(); }
  delete[] outf;
}


/* N      = Number players
 * ntrip  = Number tuples needed
 * str    = "2" or "p"
 */
template<class T>
void make_square_tuples(const T& key,int N,int ntrip,const string& str,bool zero)
{
  PRNG G;
  G.ReSeed();

  ofstream* outf=new ofstream[N];
  T a,c;
  vector<Share<T> > Sa(N),Sc(N);
  /* Generate Squares */
  for (int i=0; i<N; i++)
    { stringstream filename;
      filename << prep_data_prefix << "Squares-" << str << "-P" << i;
      cout << "Opening " << filename.str() << endl;
      outf[i].open(filename.str().c_str(),ios::out | ios::binary);
      if (outf[i].fail()) { throw file_error(filename.str().c_str()); }
    }
  for (int i=0; i<ntrip; i++)
    {
      if (!zero)
        a.randomize(G);
      make_share(Sa,a,N,key,G);
      c.mul(a,a);
      make_share(Sc,c,N,key,G);
      for (int j=0; j<N; j++)
        { Sa[j].output(outf[j],false);
          Sc[j].output(outf[j],false);
        }
    }
  for (int i=0; i<N; i++)
    { outf[i].close(); }
  delete[] outf;
}

/* N      = Number players
 * ntrip  = Number bits needed
 * str    = "2" or "p"
 */
template<class T>
void make_bits(const T& key,int N,int ntrip,const string& str,bool zero)
{
  PRNG G;
  G.ReSeed();

  ofstream* outf=new ofstream[N];
  T a;
  vector<Share<T> > Sa(N);
  /* Generate Bits */
  for (int i=0; i<N; i++)
    { stringstream filename;
      filename << prep_data_prefix << "Bits-" << str << "-P" << i;
      cout << "Opening " << filename.str() << endl;
      outf[i].open(filename.str().c_str(),ios::out | ios::binary);
      if (outf[i].fail()) { throw file_error(filename.str().c_str()); }
    }
  for (int i=0; i<ntrip; i++)
    { if ((G.get_uchar()&1)==0 || zero) { a.assign_zero(); }
      else                       { a.assign_one();  }
      make_share(Sa,a,N,key,G);
      for (int j=0; j<N; j++)
        { Sa[j].output(outf[j],false); }
    }
  for (int i=0; i<N; i++)
    { outf[i].close(); }
  delete[] outf;
}


/* N      = Number players
 * ntrip  = Number inputs needed
 * str    = "2" or "p"
 *
 */
template<class T>
void make_inputs(const T& key,int N,int ntrip,const string& str,bool zero)
{
  PRNG G;
  G.ReSeed();

  ofstream* outf=new ofstream[N];
  T a;
  vector<Share<T> > Sa(N);
  /* Generate Inputs */
  for (int player=0; player<N; player++)
    { for (int i=0; i<N; i++)
        { stringstream filename;
          filename << prep_data_prefix << "Inputs-" << str << "-P" << i << "-" << player;
          cout << "Opening " << filename.str() << endl;
          outf[i].open(filename.str().c_str(),ios::out | ios::binary);
          if (outf[i].fail()) { throw file_error(filename.str().c_str()); }
        }
      for (int i=0; i<ntrip; i++)
        {
          if (!zero)
            a.randomize(G);
          make_share(Sa,a,N,key,G);
          for (int j=0; j<N; j++)
            { Sa[j].output(outf[j],false); 
              if (j==player)
	        { a.output(outf[j],false);  }
            }
        }
      for (int i=0; i<N; i++)
        { outf[i].close(); }
    }
  delete[] outf;
}


/* N      = Number players
 * ntrip  = Number inverses needed
 * str    = "2" or "p"
 */
template<class T>
void make_inverse(const T& key,int N,int ntrip,bool zero)
{
  PRNG G;
  G.ReSeed();

  ofstream* outf=new ofstream[N];
  T a,b;
  vector<Share<T> > Sa(N),Sb(N);
  /* Generate Triples */
  for (int i=0; i<N; i++)
    { stringstream filename;
      filename << prep_data_prefix << "Inverses-" << T::type_char() << "-P" << i;
      cout << "Opening " << filename.str() << endl;
      outf[i].open(filename.str().c_str(),ios::out | ios::binary);
      if (outf[i].fail()) { throw file_error(filename.str().c_str()); }
    }
  for (int i=0; i<ntrip; i++)
    {
      if (zero)
        // ironic?
        a.assign_one();
      else
        do
          a.randomize(G);
        while (a.is_zero());
      make_share(Sa,a,N,key,G);
      b=a; b.invert();
      make_share(Sb,b,N,key,G);
      for (int j=0; j<N; j++)
        { Sa[j].output(outf[j],false); 
          Sb[j].output(outf[j],false); 
        }
    }
  for (int i=0; i<N; i++)
    { outf[i].close(); }
  delete[] outf;
}


template<class T>
void make_PreMulC(const T& key, int N, int ntrip, bool zero)
{
  stringstream ss;
  ss << prep_data_prefix << "PreMulC-" << T::type_char();
  Files<T> files(N, key, ss.str());
  PRNG G;
  G.ReSeed();
  T a, b, c;
  c = 1;
  for (int i=0; i<ntrip; i++)
    {
      // close the circle
      if (i == ntrip - 1 || zero)
        a.assign_one();
      else
        do
          a.randomize(G);
        while (a.is_zero());
      files.output_shares(a);
      b = a;
      b.invert();
      files.output_shares(b);
      files.output_shares(a * c);
      c = b;
    }
}

int main(int argc, const char** argv)
{
  insecure("preprocessing");

  ez::ezOptionParser opt;

  opt.syntax = "./Fake-Offline.x <nplayers> [OPTIONS]\n\nOptions with 2 arguments take the form '-X <#gf2n tuples>,<#modp tuples>'";
  opt.example = "./Fake-Offline.x 2 -lgp 128 -lg2 128 --default 10000\n./Fake-Offline.x 3 -trip 50000,10000 -btrip 100000\n";

  opt.add(
        "128", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Bit length of GF(p) field (default: 128)", // Help description.
        "-lgp", // Flag token.
        "--lgp" // Flag token.
  );
  opt.add(
        "40", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Bit length of GF(2^n) field (default: 40)", // Help description.
        "-lg2", // Flag token.
        "--lg2" // Flag token.
  );
  opt.add(
        "1000", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Default number of tuples to generate for ALL data types (default: 1000)", // Help description.
        "-d", // Flag token.
        "--default" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        2, // Number of args expected.
        ',', // Delimiter if expecting multiple args.
        "Number of triples, for gf2n / modp types", // Help description.
        "-trip", // Flag token.
        "--ntriples" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        2, // Number of args expected.
        ',', // Delimiter if expecting multiple args.
        "Number of random bits, for gf2n / modp types", // Help description.
        "-bit", // Flag token.
        "--nbits" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        2, // Number of args expected.
        ',', // Delimiter if expecting multiple args.
        "Number of input tuples, for gf2n / modp types", // Help description.
        "-inp", // Flag token.
        "--ninputs" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        2, // Number of args expected.
        ',', // Delimiter if expecting multiple args.
        "Number of square tuples, for gf2n / modp types", // Help description.
        "-sq", // Flag token.
        "--nsquares" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Number of inverse tuples (modp only)", // Help description.
        "-inv", // Flag token.
        "--ninverses" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Number of GF(2) triples", // Help description.
        "-btrip", // Flag token.
        "--nbittriples" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Number of GF(2) x GF(2^n) triples", // Help description.
        "-mixed", // Flag token.
        "--nbitgf2ntriples" // Flag token.
  );
  opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Set all values to zero, but not the shares", // Help description.
        "-z", // Flag token.
        "--zero" // Flag token.
  );
  opt.parse(argc, argv);

  vector<string> badOptions;
  string usage;
  unsigned int i;
  if(!opt.gotRequired(badOptions))
  {
    for (i=0; i < badOptions.size(); ++i)
      cerr << "ERROR: Missing required option " << badOptions[i] << ".";
    opt.getUsage(usage);
    cout << usage;
    return 1;
  }

  if(!opt.gotExpected(badOptions))
  {
    for(i=0; i < badOptions.size(); ++i)
      cerr << "ERROR: Got unexpected number of arguments for option " << badOptions[i] << ".";
    opt.getUsage(usage);
    cout << usage;
    return 1;
  }

  int nplayers;
  if (opt.firstArgs.size() == 2)
  {
    nplayers = atoi(opt.firstArgs[1]->c_str());
  }
  else if (opt.lastArgs.size() == 1)
  {
    nplayers = atoi(opt.lastArgs[0]->c_str());
  }
  else
  {
    cerr << "ERROR: invalid number of arguments\n";
    opt.getUsage(usage);
    cout << usage;
    return 1;
  }

  int default_num = 0;
  int ntrip2=0, ntripp=0, nbits2=0,nbitsp=0,nsqr2=0,nsqrp=0,ninp2=0,ninpp=0,ninv=0, nbittrip=0, nbitgf2ntrip=0;
  vector<int> list_options;
  int lg2, lgp;

  opt.get("--lgp")->getInt(lgp);
  opt.get("--lg2")->getInt(lg2);

  opt.get("--default")->getInt(default_num);
  ntrip2 = ntripp = nbits2 = nbitsp = nsqr2 = nsqrp = ninp2 = ninpp = ninv =
  nbittrip = nbitgf2ntrip = default_num;
  
  if (opt.isSet("--ntriples"))
  {
    opt.get("--ntriples")->getInts(list_options);
    ntrip2 = list_options[0];
    ntripp = list_options[1];
  }
  if (opt.isSet("--nbits"))
  {
    opt.get("--nbits")->getInts(list_options);
    nbits2 = list_options[0];
    nbitsp = list_options[1];
  }
  if (opt.isSet("--ninputs"))
  {
    opt.get("--ninputs")->getInts(list_options);
    ninp2 = list_options[0];
    ninpp = list_options[1];
  }
  if (opt.isSet("--nsquares"))
  {
    opt.get("--nsquares")->getInts(list_options);
    nsqr2 = list_options[0];
    nsqrp = list_options[1];
  }
  if (opt.isSet("--ninverses"))
    opt.get("--ninverses")->getInt(ninv);
  if (opt.isSet("--nbittriples"))
    opt.get("--nbittriples")->getInt(nbittrip);
  if (opt.isSet("--nbitgf2ntriples"))
    opt.get("--nbitgf2ntriples")->getInt(nbitgf2ntrip);

  bool zero = opt.isSet("--zero");
  if (zero)
      cout << "Set all values to zero" << endl;

  PRNG G;
  G.ReSeed();
  prep_data_prefix = get_prep_dir(nplayers, lgp, lg2);
  // Set up the fields
  ofstream outf;
  bigint p;
  generate_online_setup(outf, prep_data_prefix, p, lgp, lg2);

  /* Find number players and MAC keys etc*/
  gfp keyp,pp; keyp.assign_zero();
  gf2n key2,p2; key2.assign_zero();
  int tmpN = 0;
  ifstream inpf;

  // create PREP_DIR if not there
  if (mkdir_p(PREP_DIR) == -1)
  {
    cerr << "mkdir_p(" PREP_DIR ") failed\n";
    throw file_error();
  }

  for (i = 0; i < (unsigned int)nplayers; i++)
  {
      stringstream filename;
      filename << prep_data_prefix << "Player-MAC-Keys-P" << i;
      inpf.open(filename.str().c_str());
      if (inpf.fail())
      {
          inpf.close();
          cout << "No MAC key share for player " << i << ", generating a fresh one\n";
          pp.randomize(G);
          p2.randomize(G);
          ofstream outf(filename.str().c_str());
          if (outf.fail())
            throw file_error(filename.str().c_str());
          outf << nplayers << " " << pp << " " << p2;
          outf.close();
          cout << "Written new MAC key share to " << filename.str() << endl;
      }
      else
      {
        inpf >> tmpN; // not needed here
        pp.input(inpf,true);
        p2.input(inpf,true);
        inpf.close();
      }
      cout << " Key " << i << "\t p: " << pp << "\n\t 2: " << p2 << endl;
      keyp.add(pp);
      key2.add(p2);
    }
  cout << "--------------\n";
  cout << "Final Keys :\t p: " << keyp << "\n\t\t 2: " << key2 << endl;

  make_mult_triples(key2,nplayers,ntrip2,"2",zero);
  make_mult_triples(keyp,nplayers,ntripp,"p",zero);
  make_bits(key2,nplayers,nbits2,"2",zero);
  make_bits(keyp,nplayers,nbitsp,"p",zero);
  make_square_tuples(key2,nplayers,nsqr2,"2",zero);
  make_square_tuples(keyp,nplayers,nsqrp,"p",zero);
  make_inputs(key2,nplayers,ninp2,"2",zero);
  make_inputs(keyp,nplayers,ninpp,"p",zero);
  make_inverse(key2,nplayers,ninv,zero);
  make_inverse(keyp,nplayers,ninv,zero);
  make_bit_triples(key2,nplayers,nbittrip,DATA_BITTRIPLE,zero);
  make_bit_triples(key2,nplayers,nbitgf2ntrip,DATA_BITGF2NTRIPLE,zero);
  make_PreMulC(key2,nplayers,ninv,zero);
  make_PreMulC(keyp,nplayers,ninv,zero);
}
