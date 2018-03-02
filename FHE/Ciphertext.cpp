// (C) 2018 University of Bristol. See License.txt

#include "Ciphertext.h"
#include "Exceptions/Exceptions.h"

Ciphertext::Ciphertext(const FHE_PK& pk) : Ciphertext(pk.get_params())
{
}


void Ciphertext::set(const Rq_Element& a0, const Rq_Element& a1,
        const FHE_PK& pk)
{
  set(a0, a1, pk.a().get(0).get_element(0).get_limb(0));
}


word check_pk_id(word a, word b)
{
  if (a == 0)
    return b;
  else if (b == 0 or a == b)
    return a;
  else
  {
    cout << a << " vs " << b << endl;
    throw runtime_error("public keys of ciphertext operands don't match");
  }
}


void add(Ciphertext& ans,const Ciphertext& c0,const Ciphertext& c1)
{
  if (c0.params!=c1.params)  { throw params_mismatch(); }
  if (ans.params!=c1.params) { throw params_mismatch(); }
  ans.pk_id = check_pk_id(c0.pk_id, c1.pk_id);
  add(ans.cc0,c0.cc0,c1.cc0);
  add(ans.cc1,c0.cc1,c1.cc1);
}


void sub(Ciphertext& ans,const Ciphertext& c0,const Ciphertext& c1)
{
  if (c0.params!=c1.params)  { throw params_mismatch(); }
  if (ans.params!=c1.params) { throw params_mismatch(); }
  ans.pk_id = check_pk_id(c0.pk_id, c1.pk_id);
  sub(ans.cc0,c0.cc0,c1.cc0);
  sub(ans.cc1,c0.cc1,c1.cc1);
}


void mul(Ciphertext& ans,const Ciphertext& c0,const Ciphertext& c1,
         const FHE_PK& pk)
{
  if (c0.params!=c1.params)  { throw params_mismatch(); }
  if (ans.params!=c1.params) { throw params_mismatch(); }

  // Switch Modulus for c0 and c1 down to level 0
  Ciphertext cc0=c0,cc1=c1;
  cc0.Scale(pk.p()); cc1.Scale(pk.p());
  
  // Now do the multiply
  Rq_Element d0,d1,d2;

  mul(d0,cc0.cc0,cc1.cc0);
  mul(d1,cc0.cc0,cc1.cc1);
  mul(d2,cc0.cc1,cc1.cc0);
  add(d1,d1,d2);
  mul(d2,cc0.cc1,cc1.cc1); 
  d2.negate(); 

  // Now do the switch key
  d2.raise_level();
  Rq_Element t;
  d0.mul_by_p1();
  mul(t,pk.bs(),d2);
  add(d0,d0,t);

  d1.mul_by_p1();
  mul(t,pk.as(),d2);
  add(d1,d1,t);

  ans.set(d0, d1, check_pk_id(c0.pk_id, c1.pk_id));
  ans.Scale(pk.p());
}



istream& operator>>(istream& s, Ciphertext& c)
{
  int ch=s.get();
  while (isspace(ch))
    { ch = s.get(); }
  if (ch != '[')
     { throw IO_Error("Bad Ring_Element input: no '['"); }

  s >> c.pk_id;
  s >> c.cc0;

  ch=s.get();
  while (isspace(ch))
    { ch = s.get(); }
  if (ch != ',')
     { throw IO_Error("Bad Ring_Element input: no ','"); }

  s >> c.cc1;

  ch=s.get();
  while (isspace(ch))
    { ch = s.get(); }
  if (ch != ']')
     { throw IO_Error("Bad Ring_Element input: no ']'"); }

  return s;
}


template<class T,class FD,class S>
void mul(Ciphertext& ans,const Plaintext<T,FD,S>& a,const Ciphertext& c)
{
  a.to_poly();
  const vector<S>& aa=a.get_poly();

  int lev=c.cc0.level();
  Rq_Element ra((*ans.params).FFTD(),evaluation,evaluation);
  if (lev==0) { ra.lower_level(); }
  ra.from_vec(aa);
  ans.mul(c, ra);
}

void Ciphertext::mul(const Ciphertext& c, const Rq_Element& ra)
{
  if (params!=c.params) { throw params_mismatch(); }
  pk_id = c.pk_id;

  ::mul(cc0,ra,c.cc0);
  ::mul(cc1,ra,c.cc1);
}

template <>
void Ciphertext::add<0>(octetStream& os)
{
  Ciphertext tmp(*params);
  tmp.unpack(os);
  *this += tmp;
}

template <>
void Ciphertext::add<2>(octetStream& os)
{
  Ciphertext tmp(*params);
  tmp.unpack(os);
  *this += tmp;
}


template void mul(Ciphertext& ans,const Plaintext<gfp,FFT_Data,bigint>& a,const Ciphertext& c);
template void mul(Ciphertext& ans,const Plaintext<gfp,PPData,bigint>& a,const Ciphertext& c);
template void mul(Ciphertext& ans,const Plaintext<gf2n_short,P2Data,int>& a,const Ciphertext& c);


