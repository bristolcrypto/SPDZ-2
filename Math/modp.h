// (C) 2018 University of Bristol. See License.txt

#ifndef _Modp
#define _Modp

/* 
 * Currently we only support an MPIR based implementation.
 *
 * What ever is type-def'd to bigint is assumed to have
 * operator overloading for all standard operators, has
 * comparison operations and istream/ostream operators >>/<<.
 *
 * All "integer" operations will be done using operator notation
 * all "modp" operations should be done using the function calls
 * below (interchange with Montgomery arithmetic).
 *
 */

#include "Tools/octetStream.h"
#include "Tools/random.h"

#include "Math/bigint.h"
#include "Math/Zp_Data.h"

void to_bigint(bigint& ans,const modp& x,const Zp_Data& ZpD,bool reduce=true);

class modp 
{ 
  static bool rewind;

  mp_limb_t x[MAX_MOD_SZ];

  public: 

  // NEXT FUNCTION IS FOR DEBUG PURPOSES ONLY
  mp_limb_t get_limb(int i) const { return x[i]; }

  // use mem* functions instead of mpn_*, so the compiler can optimize
  modp() 
    { avx_memzero(x, sizeof(x)); }
  modp(const modp& y)
    { memcpy(x, y.x, sizeof(x)); }
  modp& operator=(const modp& y)
    { if (this!=&y) { memcpy(x, y.x, sizeof(x)); }
      return *this;
    }
  
  void assign(const char* buffer, int t) { memcpy(x, buffer, t * sizeof(mp_limb_t)); }

  void convert_destroy(bigint& source, const Zp_Data& ZpD);
  void convert_destroy(int source, const Zp_Data& ZpD) { to_modp(*this, source, ZpD); }

  void randomize(PRNG& G, const Zp_Data& ZpD);

  // Pack and unpack in native format
  //   i.e. Dont care about conversion to human readable form
  //   i.e. When we do montgomery we dont care about decoding
  void pack(octetStream& o,const Zp_Data& ZpD) const;
  void unpack(octetStream& o,const Zp_Data& ZpD);

  bool operator==(const modp& other) const { return 0 == mpn_cmp(x, other.x, MAX_MOD_SZ); }
  bool operator!=(const modp& other) const { return not (*this == other); }


   /**********************************
    *         Modp Operations        *
    **********************************/

  // Convert representation to and from a modp number
  friend void to_bigint(bigint& ans,const modp& x,const Zp_Data& ZpD,bool reduce);

  friend void to_modp(modp& ans,int x,const Zp_Data& ZpD);
  friend void to_modp(modp& ans,const bigint& x,const Zp_Data& ZpD);

  template <int T>
  friend void Add(modp& ans,const modp& x,const modp& y,const Zp_Data& ZpD);
  friend void Add(modp& ans,const modp& x,const modp& y,const Zp_Data& ZpD)
    { ZpD.Add(ans.x, x.x, y.x); }
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

  // Input and output from a stream
  //  - Can do in human or machine only format (later should be faster)
  //  - If human output appends a space to help with reading
  //    and also convert back/forth from Montgomery if needed
  void output(ostream& s,const Zp_Data& ZpD,bool human) const;
  void input(istream& s,const Zp_Data& ZpD,bool human);

  friend class gfp;

};


inline void assignZero(modp& x,const Zp_Data& ZpD)
{
  if (sizeof(x.x) <= 3 * 16)
    // use memset to allow the compiler to optimize
    // if x.x is at most 3*128 bits
    avx_memzero(x.x, sizeof(x.x));
  else
    inline_mpn_zero(x.x, ZpD.t);
}

template <int T>
inline void Add(modp& ans,const modp& x,const modp& y,const Zp_Data& ZpD)
{
  ZpD.Add<T>(ans.x, x.x, y.x);
}

inline void Sub(modp& ans,const modp& x,const modp& y,const Zp_Data& ZpD)
{
  ZpD.Sub(ans.x, x.x, y.x);
}

inline void Mul(modp& ans,const modp& x,const modp& y,const Zp_Data& ZpD)
{
  if (ZpD.montgomery)
    { ZpD.Mont_Mult(ans.x,x.x,y.x); }
  else
    { //ans.x=(x.x*y.x)%ZpD.pr;
      mp_limb_t aa[2*MAX_MOD_SZ],q[2*MAX_MOD_SZ];
      mpn_mul_n(aa,x.x,y.x,ZpD.t);
      mpn_tdiv_qr(q,ans.x,0,aa,2*ZpD.t,ZpD.prA,ZpD.t);
    }
}


#endif

