// (C) 2018 University of Bristol. See License.txt

#ifndef _bigint
#define _bigint

#include <iostream>
using namespace std;

#include <stddef.h>
#include <mpirxx.h>

#include "Exceptions/Exceptions.h"
#include "Tools/int.h"
#include "Tools/random.h"
#include "Tools/octetStream.h"
#include "Tools/avx_memcpy.h"

enum ReportType
{
  CAPACITY,
  USED,
  MINIMAL,
  REPORT_TYPE_MAX
};

class gfp;

class bigint : public mpz_class
{
public:
  bigint() : mpz_class() {}
  template <class T>
  bigint(const T& x) : mpz_class(x) {}
  bigint(const gfp& x);

  void allocate_slots(const bigint& x) { *this = x; }
  int get_min_alloc() { return get_mpz_t()->_mp_alloc; }

  void negate() { mpz_neg(get_mpz_t(), get_mpz_t()); }

#ifdef REALLOC_POLICE
  ~bigint() { lottery(); }
  void lottery();

  bigint& operator-=(const bigint& y)
  {
    if (rand() % 10000 == 0)
      if (get_mpz_t()->_mp_alloc < abs(y.get_mpz_t()->_mp_size) + 1)
        throw runtime_error("insufficient allocation");
    ((mpz_class&)*this) -= y;
    return *this;
  }
  bigint& operator+=(const bigint& y)
  {
    if (rand() % 10000 == 0)
      if (get_mpz_t()->_mp_alloc < abs(y.get_mpz_t()->_mp_size) + 1)
        throw runtime_error("insufficient allocation");
    ((mpz_class&)*this) += y;
    return *this;
  }
#endif

  int numBits() const
  { return mpz_sizeinbase(get_mpz_t(), 2); }

  void generateUniform(PRNG& G, int n_bits, bool positive = false)
  { G.get_bigint(*this, n_bits, positive); }

  void pack(octetStream& os) const { os.store(*this); }
  void unpack(octetStream& os)     { os.get(*this); };

  size_t report_size(ReportType type) const;
};


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
  return m.numBits();
}



inline int numBits(long m)
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
#ifdef REALLOC_POLICE
  if (rand() % 10000 == 0)
    if (x.get_mpz_t()->_mp_alloc < ((len + 7) / 8))
      throw runtime_error("insufficient allocation");
#endif
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

inline void inline_mpn_zero(mp_limb_t* x, mp_size_t size)
{
  avx_memzero(x, size * sizeof(mp_limb_t));
}

inline void inline_mpn_copyi(mp_limb_t* dest, const mp_limb_t* src, mp_size_t size)
{
  avx_memcpy(dest, src, size * sizeof(mp_limb_t));
}

template <class T>
int limb_size();

#endif

