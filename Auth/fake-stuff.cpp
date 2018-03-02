// (C) 2018 University of Bristol. See License.txt


#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/Share.h"
#include "Auth/fake-stuff.h"
#include "Tools/benchmarking.h"

#include <fstream>

template<class T>
void make_share(vector<Share<T> >& Sa,const T& a,int N,const T& key,PRNG& G)
{
  insecure("share generation", false);
  T mac,x,y;
  mac.mul(a,key);
  Share<T> S;
  S.set_share(a);
  S.set_mac(mac);

  Sa.resize(N);
  for (int i=0; i<N-1; i++)
    { x.randomize(G);
      y.randomize(G);
      Sa[i].set_share(x);
      Sa[i].set_mac(y);
      S.sub(S,Sa[i]);
    }
  Sa[N-1]=S;
}

template<class T>
void check_share(vector<Share<T> >& Sa,T& value,T& mac,int N,const T& key)
{
  value.assign(0);
  mac.assign(0);

  for (int i=0; i<N; i++)
    {
      value.add(Sa[i].get_share());
      mac.add(Sa[i].get_mac());
    }

  T res;
  res.mul(value, key);
  if (!res.equal(mac))
    {
      cout << "Value:      " << value << endl;
      cout << "Input MAC:  " << mac << endl;
      cout << "Actual MAC: " << res << endl;
      cout << "MAC key:    " << key << endl;
      throw mac_fail();
    }
}

template void make_share(vector<Share<gf2n> >& Sa,const gf2n& a,int N,const gf2n& key,PRNG& G);
template void make_share(vector<Share<gfp> >& Sa,const gfp& a,int N,const gfp& key,PRNG& G);

template void check_share(vector<Share<gf2n> >& Sa,gf2n& value,gf2n& mac,int N,const gf2n& key);
template void check_share(vector<Share<gfp> >& Sa,gfp& value,gfp& mac,int N,const gfp& key);

#ifdef USE_GF2N_LONG
template void make_share(vector<Share<gf2n_short> >& Sa,const gf2n_short& a,int N,const gf2n_short& key,PRNG& G);
template void check_share(vector<Share<gf2n_short> >& Sa,gf2n_short& value,gf2n_short& mac,int N,const gf2n_short& key);
#endif

// Expansion is by x=y^5+1 (as we embed GF(256) into GF(2^40)
void expand_byte(gf2n_short& a,int b)
{
  gf2n_short x,xp;
  x.assign(32+1);
  xp.assign_one();
  a.assign_zero();

  while (b!=0)
    { if ((b&1)==1)
        { a.add(a,xp); }
      xp.mul(x);
      b>>=1;
    }
}


// Have previously worked out the linear equations we need to solve
void collapse_byte(int& b,const gf2n_short& aa)
{
  word w=aa.get();
  int e35=(w>>35)&1;
  int e30=(w>>30)&1;
  int e25=(w>>25)&1;
  int e20=(w>>20)&1;
  int e15=(w>>15)&1;
  int e10=(w>>10)&1;
  int  e5=(w>>5)&1;
  int  e0=w&1;
  int a[8];
  a[7]=e35;
  a[6]=e30^a[7];
  a[5]=e25^a[7];
  a[4]=e20^a[5]^a[6]^a[7];
  a[3]=e15^a[7];
  a[2]=e10^a[3]^a[6]^a[7];
  a[1]=e5^a[3]^a[5]^a[7];
  a[0]=e0^a[1]^a[2]^a[3]^a[4]^a[5]^a[6]^a[7];

  b=0;
  for (int i=7; i>=0; i--)
    { b=b<<1;
      b+=a[i]; 
    }
}

void generate_keys(const string& directory, int nplayers)
{
  PRNG G;
  G.ReSeed();

  gf2n mac2;
  gfp macp;
  mac2.assign_zero();
  macp.assign_zero();

  for (int i = 0; i < nplayers; i++)
  {
    mac2.randomize(G);
    macp.randomize(G);
    write_mac_keys(directory, i, nplayers, macp, mac2);
  }
}

template <class T>
void write_mac_keys(const string& directory, int i, int nplayers, gfp macp, T mac2)
{
  ofstream outf;
  stringstream filename;
  if (directory.size())
    filename << directory << "/";
  filename << "Player-MAC-Keys-P" << i;
  cout << "Writing to " << filename.str().c_str() << endl;
  outf.open(filename.str().c_str());
  outf << nplayers << endl;
  macp.output(outf,true);
  outf << " ";
  mac2.output(outf,true);
  outf << endl;
  outf.close();
}

void read_keys(const string& directory, gfp& keyp, gf2n& key2, int nplayers)
{   
    gfp sharep;
    gf2n share2;
    keyp.assign_zero();
    key2.assign_zero();
    int i, tmpN = 0;
    ifstream inpf;

    for (i = 0; i < nplayers; i++)
    {
        stringstream filename;
        filename << directory << "Player-MAC-Keys-P" << i;
        inpf.open(filename.str().c_str());
        if (inpf.fail())
        {
            inpf.close();
            cout << "Error: No MAC key share found for player " << i << std::endl;
            exit(1);
        }
        else
        {
            inpf >> tmpN; // not needed here
            sharep.input(inpf,true);
            share2.input(inpf,true);
            inpf.close();
        }
        std::cout << "    Key " << i << "\t p: " << sharep << "\n\t 2: " << share2 << std::endl;
        keyp.add(sharep);
        key2.add(share2);
    }
    std::cout << "Final MAC keys :\t p: " << keyp << "\n\t\t 2: " << key2 << std::endl;
}

template void write_mac_keys(const string& directory, int i, int nplayers, gfp macp, gf2n_short mac2);
template void write_mac_keys(const string& directory, int i, int nplayers, gfp macp, gf2n_long mac2);
