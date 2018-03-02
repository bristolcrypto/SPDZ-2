// (C) 2018 University of Bristol. See License.txt


#include "Math/Setup.h"
#include "Math/gfp.h"
#include "Math/gf2n.h"

#include "Tools/mkpath.h"

#include <fstream>


/*
 * Just setup the primes, doesn't need NTL.
 * Sets idx and m to be used by SHE setup if necessary
 */
void SPDZ_Data_Setup_Primes(bigint& p,int lgp,int& idx,int& m)
{
  cout << "Setting up parameters" << endl;

  switch (lgp)
    { case -1:
        m=16;
        idx=1;    // Any old figures will do, but need to be for lgp at last
        lgp=32;   // Switch to bigger prime to get parameters
        break;
      case 32:
        m=8192;
        idx=0;
        break;
      case 64:
        m=16384;
        idx=1;
        break;
      case 128:
        m=32768; 
        idx=2;
        break;
      case 256: 
        m=32768;
        idx=3;
        break;
      case 512:
        m=65536;
        idx=4;
        break;
      default:
        m=1;
        idx=0;
        cout << "no precomputed parameters, trying anyway" << endl;
        break;
    }
  cout << "m = " << m << endl;
  generate_prime(p, lgp, m);
}

void generate_prime(bigint& p, int lgp, int m)
{
  // Here we choose a prime which is the order of a BN curve
  //    - Reason is that there are some applications where this
  //      would be a good idea. So I have hard coded it in here
  //    - This is pointless/impossible for lgp=32, 64 so for 
  //      these do something naive
  //    - Have not tested 256 and 512
  bigint u;
  int ex;
  if (lgp!=32 && lgp!=64)
    { u=1; u=u<<(lgp-1); u=sqrt(sqrt(u/36))/m;
      u=u*m;
      bigint q;
      //   cout << ex << " " << u << " " << numBits(u) << endl;
      p=(((36*u+36)*u+18)*u+6)*u+1;   // The group order of a BN curve
      q=(((36*u+36)*u+24)*u+6)*u+1;   // The base field size of a BN curve
      while (!probPrime(p) || !probPrime(q) || numBits(p)<lgp) 
        { u=u+m;
          p=(((36*u+36)*u+18)*u+6)*u+1;
          q=(((36*u+36)*u+24)*u+6)*u+1;
        }
    }
  else
    { ex=lgp-numBits(m);
      u=1; u=(u<<ex)*m;  p=u+1;
      while (!probPrime(p) || numBits(p)<lgp)
        { u=u+m;  p=u+1; }
    }
  cout << "\t p = " << p << "  u = " << u << "  :   ";
  cout << lgp << " <= " << numBits(p) << endl;
}


void generate_online_setup(ofstream& outf, string dirname, bigint& p, int lgp, int lg2)
{
  int idx, m;
  SPDZ_Data_Setup_Primes(p, lgp, idx, m);
  write_online_setup(outf, dirname, p, lg2);
}

void write_online_setup(ofstream& outf, string dirname, const bigint& p, int lg2)
{
  if (p == 0)
    throw runtime_error("prime cannot be 0");

  stringstream ss;
  ss << dirname;
  cout << "Writing to file in " << ss.str() << endl;
  // create preprocessing dir. if necessary
  if (mkdir_p(ss.str().c_str()) == -1)
  {
    cerr << "mkdir_p(" << ss.str() << ") failed\n";
    throw file_error();
  }

  // Output the data
  ss << "/Params-Data";
  outf.open(ss.str().c_str());
  // Only need p and lg2 for online phase
  outf << p << endl;
  // Fix as a negative lg2 is a ``signal'' to choose slightly weaker
  // LWE parameters
  outf << abs(lg2) << endl;

  gfp::init_field(p, true);
  gf2n::init_field(lg2);
}

string get_prep_dir(int nparties, int lg2p, int gf2ndegree)
{
  if (gf2ndegree == 0)
    gf2ndegree = gf2n::default_length();
  stringstream ss;
  ss << PREP_DIR << nparties << "-" << lg2p << "-" << gf2ndegree << "/";
  return ss.str();
}

// Only read enough to initialize the fields (i.e. for OT offline or online phase only)
void read_setup(const string& dir_prefix)
{
  int lg2;
  bigint p;

  string filename = dir_prefix + "Params-Data";
  cerr << "loading params from: " << filename << endl;

  // backwards compatibility hack
  if (dir_prefix.compare("") == 0)
    filename = string(PREP_DIR "Params-Data");

  ifstream inpf(filename.c_str());
  if (inpf.fail()) { throw file_error(filename.c_str()); }
  inpf >> p;
  inpf >> lg2;

  inpf.close();

  gfp::init_field(p);
  gf2n::init_field(lg2);
}

void read_setup(int nparties, int lg2p, int gf2ndegree)
{
  string dir = get_prep_dir(nparties, lg2p, gf2ndegree);
  read_setup(dir);
}
