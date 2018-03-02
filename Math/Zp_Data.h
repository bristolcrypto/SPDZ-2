// (C) 2018 University of Bristol. See License.txt

#ifndef _Zp_Data
#define _Zp_Data

/* Class to define helper information for a Zp element
 *
 * Basically the data needed for Montgomery operations 
 *
 * Almost all data is public as this is basically a container class
 *
 */

#include "Math/bigint.h"
#include "Tools/random.h"

#include <smmintrin.h>
#include <iostream>
using namespace std;

#ifndef MAX_MOD_SZ
   #ifdef LargeM
     #define MAX_MOD_SZ 20
   #else
     #define MAX_MOD_SZ 2
  #endif
#endif

class modp;

class Zp_Data
{
  bool        montgomery;  // True if we are using Montgomery arithmetic
  mp_limb_t   R[MAX_MOD_SZ],R2[MAX_MOD_SZ],R3[MAX_MOD_SZ],pi;
  // extra limb needed for Montgomery multiplication
  mp_limb_t   prA[MAX_MOD_SZ+1];
  int         t;           // More Montgomery data

  void Mont_Mult(mp_limb_t* z,const mp_limb_t* x,const mp_limb_t* y) const;

  public:

  bigint       pr;
  mp_limb_t    mask;

  void assign(const Zp_Data& Zp);
  void init(const bigint& p,bool mont=true);
  int get_t() const { return t; }
  const mp_limb_t* get_prA() const { return prA; }

  void pack(octetStream& o) const;
  void unpack(octetStream& o);

  // This one does nothing, needed so as to make vectors of Zp_Data
  Zp_Data() : montgomery(0), pi(0), mask(0) { t=1; }

  // The main init funciton
  Zp_Data(const bigint& p,bool mont=true)
    { init(p,mont); }

  Zp_Data(const Zp_Data& Zp) { assign(Zp); }
  Zp_Data& operator=(const Zp_Data& Zp) 
    { if (this!=&Zp) { assign(Zp); }
      return *this;
    }
  ~Zp_Data()  { ; }

  template <int T>
  void Add(mp_limb_t* ans,const mp_limb_t* x,const mp_limb_t* y) const;
  void Add(mp_limb_t* ans,const mp_limb_t* x,const mp_limb_t* y) const;
  void Sub(mp_limb_t* ans,const mp_limb_t* x,const mp_limb_t* y) const;

  __m128i get_random128(PRNG& G);

  bool operator!=(const Zp_Data& other) const;

   friend void to_bigint(bigint& ans,const modp& x,const Zp_Data& ZpD,bool reduce);

   friend void to_modp(modp& ans,int x,const Zp_Data& ZpD);
   friend void to_modp(modp& ans,const bigint& x,const Zp_Data& ZpD);

   friend void Add(modp& ans,const modp& x,const modp& y,const Zp_Data& ZpD);
   friend void Sub(modp& ans,const modp& x,const modp& y,const Zp_Data& ZpD);
   friend void Mul(modp& ans,const modp& x,const modp& y,const Zp_Data& ZpD);
   friend void Sqr(modp& ans,const modp& x,const Zp_Data& ZpD);
   friend void Negate(modp& ans,const modp& x,const Zp_Data& ZpD);
   friend void Inv(modp& ans,const modp& x,const Zp_Data& ZpD);

   friend void Power(modp& ans,const modp& x,int exp,const Zp_Data& ZpD);
   friend void Power(modp& ans,const modp& x,const bigint& exp,const Zp_Data& ZpD);

   friend void assignOne(modp& x,const Zp_Data& ZpD);
   friend void assignZero(modp& x,const Zp_Data& ZpD);
   friend bool isZero(const modp& x,const Zp_Data& ZpD);
   friend bool isOne(const modp& x,const Zp_Data& ZpD);
   friend bool areEqual(const modp& x,const modp& y,const Zp_Data& ZpD);

   friend class modp;

   friend ostream& operator<<(ostream& s,const Zp_Data& ZpD);
   friend istream& operator>>(istream& s,Zp_Data& ZpD);
};

template<>
inline void Zp_Data::Add<0>(mp_limb_t* ans,const mp_limb_t* x,const mp_limb_t* y) const
{
  mp_limb_t carry = mpn_add_n(ans,x,y,t);
  if (carry!=0 || mpn_cmp(ans,prA,t)>=0)
    { mpn_sub_n(ans,ans,prA,t); }
}

template<>
inline void Zp_Data::Add<2>(mp_limb_t* ans,const mp_limb_t* x,const mp_limb_t* y) const
{
#ifdef __clang__
  Add<0>(ans, x, y);
#else
  __uint128_t a, b, p;
  memcpy(&a, x, sizeof(__uint128_t));
  memcpy(&b, y, sizeof(__uint128_t));
  memcpy(&p, prA, sizeof(__uint128_t));
  __uint128_t c = a + b;
  asm goto ("jc %l[sub]" :::: sub);
  if (c >= p)
 sub:
      c -= p;
  memcpy(ans, &c, sizeof(__uint128_t));
#endif
}

inline void Zp_Data::Add(mp_limb_t* ans,const mp_limb_t* x,const mp_limb_t* y) const
{
  if (t == 2)
    return Add<2>(ans, x, y);
  else
    return Add<0>(ans, x, y);
}

inline void Zp_Data::Sub(mp_limb_t* ans,const mp_limb_t* x,const mp_limb_t* y) const
{
  mp_limb_t borrow = mpn_sub_n(ans,x,y,t);
  if (borrow!=0)
    mpn_add_n(ans,ans,prA,t);
}

#endif
