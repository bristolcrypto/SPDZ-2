// (C) 2017 University of Bristol. See License.txt

#ifndef _bigint
#define _bigint

#include <iostream>
using namespace std;

#include <stddef.h>
#include <mpirxx.h>

typedef mpz_class bigint;

#include "Exceptions/Exceptions.h"
#include "Tools/int.h"

/**********************************
 *       Utility Functions        *
 **********************************/

inline int gcd(const int x,const int y)
{
  bigint xx=x;
  return mpz_gcd_ui(NULL,xx.get_mpz_t(),y);
}


inline bigint gcd(const bigint& x,const bigint& y)
{ 
  bigint g;
  mpz_gcd(g.get_mpz_t(),x.get_mpz_t(),y.get_mpz_t());
  return g;
}


inline void invMod(bigint& ans,const bigint& x,const bigint& p)
{
  mpz_invert(ans.get_mpz_t(),x.get_mpz_t(),p.get_mpz_t());
}

inline int numBits(const bigint& m)
{
  return mpz_sizeinbase(m.get_mpz_t(),2);
}



inline int numBits(int m)
{
  bigint te=m;
  return mpz_sizeinbase(te.get_mpz_t(),2);
}



inline int numBytes(const bigint& m)
{
  return mpz_sizeinbase(m.get_mpz_t(),256);
}





inline int probPrime(const bigint& x)
{
  gmp_randstate_t rand_state;
  gmp_randinit_default(rand_state);
  int ans=mpz_probable_prime_p(x.get_mpz_t(),rand_state,40,0);
  gmp_randclear(rand_state);
  return ans;
}


inline void bigintFromBytes(bigint& x,octet* bytes,int len)
{
  mpz_import(x.get_mpz_t(),len,1,sizeof(octet),0,0,bytes);
}


inline void bytesFromBigint(octet* bytes,const bigint& x,unsigned int len)
{
  size_t ll;
  mpz_export(bytes,&ll,1,sizeof(octet),0,0,x.get_mpz_t());
  if (ll>len)
    { throw invalid_length(); }
  for (unsigned int i=ll; i<len; i++)
    { bytes[i]=0; }
}


inline int isOdd(const bigint& x)
{
  return mpz_odd_p(x.get_mpz_t());
}


bigint sqrRootMod(const bigint& x,const bigint& p);

bigint powerMod(const bigint& x,const bigint& e,const bigint& p);

// Assume e>=0
int powerMod(int x,int e,int p);

inline int Hwt(int N)
{
  int result=0;
  while(N)
    { result++;
      N&=(N-1);
    }
  return result;
}

#endif

