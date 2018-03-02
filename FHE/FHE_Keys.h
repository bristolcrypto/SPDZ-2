// (C) 2018 University of Bristol. See License.txt

#ifndef _FHE_Keys
#define _FHE_Keys

/* These are standardly generated FHE public and private key pairs */

#include "FHE/Rq_Element.h"
#include "FHE/FHE_Params.h"
#include "FHE/Random_Coins.h"
#include "FHE/Ciphertext.h"
#include "FHE/Plaintext.h"

class FHE_PK;
class Ciphertext;

class FHE_SK
{
  Rq_Element sk;
  const FHE_Params *params;
  bigint pr;

  public:

  const FHE_Params& get_params() const { return *params; }

  bigint p() const { return pr; }

  // secret key always on lower level
  void assign(const Rq_Element& s) { sk=s; sk.lower_level(); }

  FHE_SK(const FHE_Params& pms, const bigint& p = 0)
    : sk(pms.FFTD(),evaluation,evaluation) { params=&pms; pr=p; }

  FHE_SK(const FHE_PK& pk);

  // Rely on default copy constructor/assignment
  
  const Rq_Element& s() const { return sk; }

  // Assumes params is set out of band
  friend ostream& operator<<(ostream& s,const FHE_SK& SK)
    { s << SK.sk; return s; }
  friend istream& operator>>(istream& s, FHE_SK& SK)
    { s >> SK.sk; return s; }
  
  void pack(octetStream& os) const { sk.pack(os); pr.pack(os); }
  void unpack(octetStream& os)     { sk.unpack(os); pr.unpack(os); }

  // Assumes Ring and prime of mess have already been set correctly
  // Ciphertext c must be at level 0 or an error occurs
  //            c must have same params as SK
  void decrypt(Plaintext<gfp,FFT_Data,bigint>& mess,const Ciphertext& c) const;
  void decrypt(Plaintext<gfp,PPData,bigint>& mess,const Ciphertext& c) const;
  void decrypt(Plaintext<gf2n_short,P2Data,int>& mess,const Ciphertext& c) const;

  template <class FD>
  Plaintext<typename FD::T, FD, typename FD::S> decrypt(const Ciphertext& c, const FD& FieldD);

  template <class FD>
  void decrypt_any(Plaintext_<FD>& mess, const Ciphertext& c);

  // Three stage procedure for Distributed Decryption
  //  - First stage produces my shares
  //  - Second stage adds in another players shares, do this once for each other player
  //  - Third stage outputs the message by executing
  //        mess.set_poly_mod(vv,mod)  
  //    where mod p0 and mess is Plaintext<T,FD,S>
  void dist_decrypt_1(vector<bigint>& vv,const Ciphertext& ctx,int player_number,int num_players) const;
  void dist_decrypt_2(vector<bigint>& vv,const vector<bigint>& vv1) const;
  

  friend void KeyGen(FHE_PK& PK,FHE_SK& SK,PRNG& G);
  
  /* Add secret key onto the existing one
   *   Used for adding distributed keys together
   *   a,b,c must have same params otherwise an error
   */
  friend void add(FHE_SK& a,const FHE_SK& b,const FHE_SK& c);

  FHE_SK operator+(const FHE_SK& x) { FHE_SK res(*params, pr); add(res, *this, x); return res; }
  FHE_SK& operator+=(const FHE_SK& x) { add(*this, *this, x); return *this; }

  bool operator!=(const FHE_SK& x) { return pr != x.pr or sk != x.sk; }

  void check(const FHE_Params& params, const FHE_PK& pk, const bigint& pr) const;
};


class FHE_PK
{
  Rq_Element a0,b0;
  Rq_Element Sw_a,Sw_b;
  const FHE_Params *params;
  bigint pr;

  public:

  const FHE_Params& get_params() const { return *params; }

  bigint p() const { return pr; }

  void assign(const Rq_Element& a,const Rq_Element& b,
              const Rq_Element& sa,const Rq_Element& sb
             )
	{ a0=a; b0=b; Sw_a=sa; Sw_b=sb; }

 
  FHE_PK(const FHE_Params& pms, const bigint& p = 0)
    : a0(pms.FFTD(),evaluation,evaluation),
      b0(pms.FFTD(),evaluation,evaluation),
      Sw_a(pms.FFTD(),evaluation,evaluation), 
      Sw_b(pms.FFTD(),evaluation,evaluation) 
       { params=&pms; pr=p; }

  // Rely on default copy constructor/assignment
  
  const Rq_Element& a() const { return a0; }
  const Rq_Element& b() const { return b0; }

  const Rq_Element& as() const { return Sw_a; }
  const Rq_Element& bs() const { return Sw_b; }

  
  // c must have same params as PK and rc
  void encrypt(Ciphertext& c, const Plaintext<gfp,FFT_Data,bigint>& mess, const Random_Coins& rc) const;
  void encrypt(Ciphertext& c, const Plaintext<gfp,PPData,bigint>& mess, const Random_Coins& rc) const;
  void encrypt(Ciphertext& c, const Plaintext<gf2n_short,P2Data,int>& mess, const Random_Coins& rc) const;

  template <class S>
  void encrypt(Ciphertext& c, const vector<S>& mess, const Random_Coins& rc) const;

  void quasi_encrypt(Ciphertext& c, const Rq_Element& mess, const Random_Coins& rc) const;

  template <class FD>
  Ciphertext encrypt(const Plaintext<typename FD::T, FD, typename FD::S>& mess, const Random_Coins& rc) const;
  template <class FD>
  Ciphertext encrypt(const Plaintext<typename FD::T, FD, typename FD::S>& mess) const;

  friend void KeyGen(FHE_PK& PK,FHE_SK& SK,PRNG& G);

  Rq_Element sample_secret_key(PRNG& G);
  void KeyGen(Rq_Element& sk, PRNG& G, int noise_boost = 1);

  void check_noise(const FHE_SK& sk);
  void check_noise(const Rq_Element& x, bool check_modulo = false);

  // params setting is done out of these IO/pack/unpack functions

  friend ostream& operator<<(ostream& s,const FHE_PK& PK)
    { s << PK.a0 << PK.b0 << PK.Sw_a << PK.Sw_b; return s; }
  friend istream& operator>>(istream& s, FHE_PK& PK)
    { s >> PK.a0 >> PK.b0 >> PK.Sw_a >> PK.Sw_b; return s; }

  void pack(octetStream& o) const
    { a0.pack(o); b0.pack(o); Sw_a.pack(o); Sw_b.pack(o); pr.pack(o); }
  void unpack(octetStream& o) 
    { a0.unpack(o); b0.unpack(o); Sw_a.unpack(o); Sw_b.unpack(o); pr.unpack(o); }
  
  bool operator!=(const FHE_PK& x) const;

  void check(const FHE_Params& params, const bigint& pr) const;
};


// PK and SK must have the same params, otherwise an error
void KeyGen(FHE_PK& PK,FHE_SK& SK,PRNG& G);

#endif
