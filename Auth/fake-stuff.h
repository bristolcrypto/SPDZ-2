// (C) 2018 University of Bristol. See License.txt


#ifndef _fake_stuff
#define _fake_stuff

#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/Share.h"

#include <fstream>
using namespace std;

template<class T>
void make_share(vector<Share<T> >& Sa,const T& a,int N,const T& key,PRNG& G);

template<class T>
void check_share(vector<Share<T> >& Sa,T& value,T& mac,int N,const T& key);

void expand_byte(gf2n_short& a,int b);
void collapse_byte(int& b,const gf2n_short& a);

// Generate MAC key shares
void generate_keys(const string& directory, int nplayers);

template <class T>
void write_mac_keys(const string& directory, int player_num, int nplayers, gfp keyp, T key2);

// Read MAC key shares and compute keys
void read_keys(const string& directory, gfp& keyp, gf2n& key2, int nplayers);

template <class T>
class Files
{
public:
  ofstream* outf;
  int N;
  T key;
  PRNG G;
  Files(int N, const T& key, const string& prefix) : N(N), key(key)
  {
    outf = new ofstream[N];
    for (int i=0; i<N; i++)
      {
        stringstream filename;
        filename << prefix << "-P" << i;
        cout << "Opening " << filename.str() << endl;
        outf[i].open(filename.str().c_str(),ios::out | ios::binary);
        if (outf[i].fail())
          throw file_error(filename.str().c_str());
      }
    G.ReSeed();
  }
  ~Files()
  {
    delete[] outf;
  }
  void output_shares(const T& a)
  {
    vector<Share<T> > Sa(N);
    make_share(Sa,a,N,key,G);
    for (int j=0; j<N; j++)
      Sa[j].output(outf[j],false);
  }
};

#endif
