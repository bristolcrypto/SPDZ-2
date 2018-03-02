// (C) 2018 University of Bristol. See License.txt


#include "FHE/Plaintext.h"
#include "FHE/Ring_Element.h"


template<>
void Plaintext<gfp,FFT_Data,bigint>::from_poly() const
{
  if (type!=Polynomial) { return; }

  Ring_Element e(*Field_Data,polynomial);
  e.from_vec(b);
  e.change_rep(evaluation);
  a.resize(n_slots);
  for (unsigned int i=0; i<a.size(); i++)
     { a[i]=e.get_element(i); }
  type=Both;
}


template<>
void Plaintext<gfp,FFT_Data,bigint>::to_poly() const
{
  if (type!=Evaluation) { return; }

  Ring_Element e(*Field_Data,evaluation);
  for (unsigned int i=0; i<a.size(); i++)
    { e.set_element(i,a[i].get()); }
  e.change_rep(polynomial);
  e.to_vec_bigint(b);
  type=Both;
}


template<>
void Plaintext<gfp,PPData,bigint>::from_poly() const
{
  if (type!=Polynomial) { return; }
  vector<modp> aa((*Field_Data).phi_m());
  for (unsigned int i=0; i<aa.size(); i++)
    { to_modp(aa[i],b[i],(*Field_Data).prData); }
  (*Field_Data).to_eval(aa);
  a.resize(n_slots);
  for (unsigned int i=0; i<aa.size(); i++)
    { a[i].assign(aa[i]); }
  type=Both;
}


template<>
void Plaintext<gfp,PPData,bigint>::to_poly() const
{
  if (type!=Evaluation) { return; }
  cout << "This is VERY inefficient to convert a plaintext to poly representation" << endl;
  vector<modp> bb((*Field_Data).phi_m());
  for (unsigned int i=0; i<bb.size(); i++)
    { bb[i]=a[i].get(); }
  (*Field_Data).from_eval(bb);
  for (unsigned int i=0; i<bb.size(); i++)
    { to_bigint(b[i],bb[i],(*Field_Data).prData); }
  type=Both;
}



template<>
void Plaintext<gf2n_short,P2Data,int>::from_poly() const
{ 
  if (type!=Polynomial) { return; }
  a.resize(n_slots);
  (*Field_Data).backward(a,b); 
  type=Both;
}


template<>
void Plaintext<gf2n_short,P2Data,int>::to_poly() const
{ 
  if (type!=Evaluation) { return; }
  (*Field_Data).forward(b,a); 
  type=Both;
}



template<>
void Plaintext<gfp,FFT_Data,bigint>::set_sizes()
{ n_slots = (*Field_Data).phi_m();
  degree = n_slots;
}


template<>
void Plaintext<gfp,PPData,bigint>::set_sizes()
{ n_slots = (*Field_Data).phi_m();
  degree = n_slots;
}


template<>
void Plaintext<gf2n_short,P2Data,int>::set_sizes()
{ n_slots = (*Field_Data).num_slots();
  degree = (*Field_Data).degree();
}


template<class T, class FD, class S>
void Plaintext<T, FD, S>::allocate(PT_Type type) const
{
  if (type != Evaluation)
    b.resize(degree);
  if (type != Polynomial)
    a.resize(n_slots);
  this->type = type;
}


template<class T, class FD, class S>
void Plaintext<T, FD, S>::allocate_slots(const bigint& value)
{
  b.resize(degree);
  for (auto& x : b)
    x = value;
}

template<>
void Plaintext<gf2n_short, P2Data, int>::allocate_slots(const bigint& value)
{
  // nothing to allocate for int
  (void)value;
}

template<>
int Plaintext<gfp, FFT_Data, bigint>::get_min_alloc()
{
  int res = 1 << 30;
  for (auto& x : b)
    res = min(res, x.get_min_alloc());
  return res;
}


void signed_mod(bigint& x, const bigint& mod, const bigint& half_mod, const bigint& dest_mod)
{
  if (x > half_mod)
    x -= mod;
  x %= dest_mod;
  if (x < 0)
    x += dest_mod;
}

template<>
void Plaintext<gfp,FFT_Data,bigint>::set_poly_mod(const Generator<bigint>& generator,const bigint& mod)
{
  allocate(Polynomial);
  bigint half_mod = mod / 2;
  for (unsigned int i=0; i<b.size(); i++)
    {
      generator.get(b[i]);
      signed_mod(b[i], mod, half_mod, Field_Data->get_prime());
    }
}


template<>
void Plaintext<gfp,FFT_Data,bigint>::set_poly_mod(const vector<bigint>& vv,const bigint& mod)
{
  b = vv;
  vector<bigint>& pol = b;
  bigint half_mod = mod / 2;
  for (unsigned int i=0; i<vv.size(); i++)
    {
      pol[i] = vv[i];
      if (pol[i] > half_mod)
        pol[i] -= mod;
      pol[i] %= (*Field_Data).get_prime();
      if (pol[i]<0) { pol[i]+=(*Field_Data).get_prime(); }
    }
  type = Polynomial;
}


template<>
void Plaintext<gfp,PPData,bigint>::set_poly_mod(const vector<bigint>& vv,const bigint& mod)
{
  b = vv;
  vector<bigint>& pol = b;
  for (unsigned int i=0; i<vv.size(); i++)
    { if (vv[i]>mod/2) { pol[i]=vv[i]-mod; }
      else             { pol[i]=vv[i];     }
      pol[i]=pol[i]%(*Field_Data).get_prime();
      if (pol[i]<0) { pol[i]+=(*Field_Data).get_prime(); }
    }
  type = Polynomial;
}




template<>
void Plaintext<gf2n_short,P2Data,int>::set_poly_mod(const vector<bigint>& vv,const bigint& mod)
{
  vector<int> pol(vv.size());
  bigint te;
  for (unsigned int i=0; i<vv.size(); i++)
    { if (vv[i]>mod/2) { te=vv[i]-mod; }
      else             { te=vv[i];     }
      pol[i]=isOdd(te);
    }
  set_poly(pol);
}


void rand_poly(vector<bigint>& b,PRNG& G,const bigint& pr,bool positive=true)
{
  for (unsigned int i=0; i<b.size(); i++)
    {
      G.randomBnd(b[i], pr, positive);
    }
}

void rand_poly(vector<int>& b,PRNG& G,const bigint& pr,bool positive=true)
{
  (void)positive;
  if (pr!=2) { throw bad_value(); }
  int l=0;
  unsigned char ch=0;
  for (unsigned int i=0; i<b.size(); i++)
    { if (l==0)
        { ch=G.get_uchar(); l=8; }
      b[i]=ch&1;
      ch>>=1; l--;
    }
}

template<class T,class FD,class S>
void Plaintext<T,FD,S>::randomize(PRNG& G,condition cond)
{
  switch (cond)
    { case Full:
        rand_poly(b,G,(*Field_Data).get_prime());
        type=Polynomial;
        break;
      case Diagonal:
        a.resize(n_slots);
        a[0].randomize(G);
        for (unsigned int i=1; i<a.size(); i++)
           { a[i]=a[0]; }
        type=Evaluation;
        break;
      default:
        // Gen a plaintext with 0/1 in each slot
        int nb=0; 
        unsigned char ch=0;
        a.resize(n_slots);
	for (unsigned int i=0; i<a.size(); i++)
           { if (nb==0)
               { ch=G.get_uchar();
	         nb=8;
               }
             if ((ch&1)==1)
		{ a[i].assign_one(); }
             else
	        { a[i].assign_zero(); }
	     ch=ch>>1; nb--;
           }
        type=Evaluation;
        break;
    }
    
}


template<>
void Plaintext_<FFT_Data>::randomize(PRNG& G, bigint B, bool Diag, bool binary, PT_Type t)
{
  if (Diag or binary)
    throw not_implemented();
  if (B == 0)
    throw runtime_error("cannot randomize modulo 0");

  allocate(t);
  switch (t)
  {
    case Polynomial:
      rand_poly(b, G, B, false);
      break;
    case Evaluation:
      for (int i = 0; i < n_slots; i++)
        a[i] = G.randomBnd(B);
      break;
    default:
      throw runtime_error("wrong type for randomization with bound");
      break;
  }
}


template<class T,class FD,class S>
void Plaintext<T,FD,S>::randomize(PRNG& G, int n_bits, bool Diag, bool binary, PT_Type t)
{
  if (Diag or binary)
    throw not_implemented();

  allocate(t);
  switch(t)
  {
    case Polynomial:
      for (int i = 0; i < n_slots; i++)
        G.get(b[i], n_bits, false);
      break;
    default:
      throw not_implemented();
  }
}


template<class T,class FD,class S>
ostream& operator<<(ostream& s,const Plaintext<T,FD,S>& e)
{
  e.to_poly();
  s << "[";
  bigint te;
  for (unsigned int i=0; i<e.b.size(); i++) 
    { s << e.b[i] << " "; }
  s << "]";
  return s;
}


template<class T,class FD,class S>
istream& operator>>(istream& s, Plaintext<T,FD,S>& e)
{
  int ch = s.get();
  while (isspace(ch)) { ch = s.get(); }
  
  if (ch != '[')
     { throw IO_Error("Bad Plaintext input: no '['"); }
  for (unsigned int i=0; i<e.b.size(); i++)                 
    { s >> e.b[i]; }
  ch=s.get();
  while (isspace(ch)) { ch = s.get(); }

  if (ch != ']')
     { throw IO_Error("Bad Plaintext input: no ']'"); }

  e.type=Polynomial;

  return s;
}


template<class T,class FD,class S>
void Plaintext<T,FD,S>::assign_zero(PT_Type t)
{ 
  type=t;
  allocate();
  if (type!=Polynomial)
    {
      a.resize(n_slots);
      for (unsigned int i=0; i<a.size(); i++)
        { a[i].assign_zero(); }
    }
  if (type!=Evaluation)
    { for (unsigned int i=0; i<b.size(); i++)
        { b[i]=0; }
    }

}

template<class T,class FD,class S>
void Plaintext<T,FD,S>::assign_one(PT_Type t)
{ 
  type=t;
  allocate();
  if (type!=Polynomial)
    {
      a.resize(n_slots);
      for (unsigned int i=0; i<a.size(); i++)
        { a[i].assign_one(); }
    }
  if (type!=Evaluation)
    { for (unsigned int i=1; i<b.size(); i++)
        { b[i]=0; }
      b[0]=1;
    }
}


template<class T,class FD,class S>
Plaintext<T,FD,S>& Plaintext<T,FD,S>::operator+=(
    const Plaintext<T,FD,S>& y)
{
  if (Field_Data!=y.Field_Data)  { throw field_mismatch(); }

  to_poly();
  y.to_poly();

  if (b.size() != y.b.size())
    throw length_error("size mismatch");

  for (unsigned int i = 0; i < b.size(); i++)
    b[i] += y.b[i];

  type = Polynomial;
  return *this;
}


template<>
void add(Plaintext<gfp,FFT_Data,bigint>& z,const Plaintext<gfp,FFT_Data,bigint>& x,
                                           const Plaintext<gfp,FFT_Data,bigint>& y)
{
  if (z.Field_Data!=x.Field_Data)  { throw field_mismatch(); }
  if (z.Field_Data!=y.Field_Data)  { throw field_mismatch(); }

  if (x.type==Both && y.type!=Both)      { z.type=y.type; }
  else if (y.type==Both && x.type!=Both) { z.type=x.type; }
  else if (x.type!=y.type)               { throw rep_mismatch(); }
  else                                   { z.type=x.type; }

  z.allocate();
  if (z.type!=Polynomial)
    {
      z.a.resize(z.n_slots);
      for (unsigned int i=0; i<z.a.size(); i++)
        { z.a[i].add(x.a[i],y.a[i]); }
    }
  if (z.type!=Evaluation)
    { for (unsigned int i=0; i<z.b.size(); i++)
        { z.b[i]=x.b[i]+y.b[i];
          if (z.b[i]>(*z.Field_Data).get_prime())
	    { z.b[i]-=(*z.Field_Data).get_prime(); }
	}
    }
}


template<>
void add(Plaintext<gfp,PPData,bigint>& z,const Plaintext<gfp,PPData,bigint>& x,
                                         const Plaintext<gfp,PPData,bigint>& y)
{
  if (z.Field_Data!=x.Field_Data)  { throw field_mismatch(); }
  if (z.Field_Data!=y.Field_Data)  { throw field_mismatch(); }

  if (x.type==Both && y.type!=Both)      { z.type=y.type; }
  else if (y.type==Both && x.type!=Both) { z.type=x.type; }
  else if (x.type!=y.type)               { throw rep_mismatch(); }
  else                                   { z.type=x.type; }

  if (z.type!=Polynomial)
    {
      z.a.resize(z.n_slots);
      for (unsigned int i=0; i<z.a.size(); i++)
        { z.a[i].add(x.a[i],y.a[i]); }
    }
  if (z.type!=Evaluation)
    { for (unsigned int i=0; i<z.b.size(); i++)
        { z.b[i]=x.b[i]+y.b[i];
          if (z.b[i]>(*z.Field_Data).get_prime())
	    { z.b[i]-=(*z.Field_Data).get_prime(); }
	}
    }
}




template<>
void add(Plaintext<gf2n_short,P2Data,int>& z,const Plaintext<gf2n_short,P2Data,int>& x,
                                       const Plaintext<gf2n_short,P2Data,int>& y)
{
  if (z.Field_Data!=x.Field_Data)  { throw field_mismatch(); }
  if (z.Field_Data!=y.Field_Data)  { throw field_mismatch(); }

  if (x.type==Both && y.type!=Both)      { z.type=y.type; }
  else if (y.type==Both && x.type!=Both) { z.type=x.type; }
  else if (x.type!=y.type)               { throw rep_mismatch(); }
  else                                   { z.type=x.type; }

  z.allocate();
  if (z.type!=Polynomial)
    {
      z.a.resize(z.n_slots);
      for (unsigned int i=0; i<z.a.size(); i++)
        { z.a[i].add(x.a[i],y.a[i]); }
    }
  if (z.type!=Evaluation)
    { for (unsigned int i=0; i<z.b.size(); i++)
        { z.b[i]=x.b[i]+y.b[i];
          if (z.b[i]==2)
            { z.b[i]=0; }
        }
    }
}


template<>
void sub(Plaintext<gfp,FFT_Data,bigint>& z,const Plaintext<gfp,FFT_Data,bigint>& x,
                                           const Plaintext<gfp,FFT_Data,bigint>& y)
{
  if (z.Field_Data!=x.Field_Data)  { throw field_mismatch(); }
  if (z.Field_Data!=y.Field_Data)  { throw field_mismatch(); }

  if (x.type==Both && y.type!=Both)      { z.type=y.type; }
  else if (y.type==Both && x.type!=Both) { z.type=x.type; }
  else if (x.type!=y.type)               { throw rep_mismatch(); }
  else                                   { z.type=x.type; }

  z.allocate();
  if (z.type!=Polynomial)
    {
      z.a.resize(z.n_slots);
      for (unsigned int i=0; i<z.a.size(); i++)
        { z.a[i].sub(x.a[i],y.a[i]); }
    }
  if (z.type!=Evaluation)
    { for (unsigned int i=0; i<z.b.size(); i++)
        {
          z.b[i]=x.b[i];
          z.b[i]-=y.b[i];
          if (z.b[i]<0)
            { z.b[i]+=(*z.Field_Data).get_prime(); }
        }
    }
}



template<>
void sub(Plaintext<gfp,PPData,bigint>& z,const Plaintext<gfp,PPData,bigint>& x,
                                         const Plaintext<gfp,PPData,bigint>& y)
{
  if (z.Field_Data!=x.Field_Data)  { throw field_mismatch(); }
  if (z.Field_Data!=y.Field_Data)  { throw field_mismatch(); }

  if (x.type==Both && y.type!=Both)      { z.type=y.type; }
  else if (y.type==Both && x.type!=Both) { z.type=x.type; }
  else if (x.type!=y.type)               { throw rep_mismatch(); }
  else                                   { z.type=x.type; }

  z.allocate();
  if (z.type!=Polynomial)
    {
      z.a.resize(z.n_slots);
      for (unsigned int i=0; i<z.a.size(); i++)
        { z.a[i].sub(x.a[i],y.a[i]); }
    }
  if (z.type!=Evaluation)
    { for (unsigned int i=0; i<z.b.size(); i++)
        { z.b[i]=x.b[i]-y.b[i];
          if (z.b[i]<0)
            { z.b[i]+=(*z.Field_Data).get_prime(); }
        }
    }
}





template<>
void sub(Plaintext<gf2n_short,P2Data,int>& z,const Plaintext<gf2n_short,P2Data,int>& x,
                                       const Plaintext<gf2n_short,P2Data,int>& y)
{
  if (z.Field_Data!=x.Field_Data)  { throw field_mismatch(); }
  if (z.Field_Data!=y.Field_Data)  { throw field_mismatch(); }

  if (x.type==Both && y.type!=Both)      { z.type=y.type; }
  else if (y.type==Both && x.type!=Both) { z.type=x.type; }
  else if (x.type!=y.type)               { throw rep_mismatch(); }
  else                                   { z.type=x.type; }

  z.allocate();
  if (z.type!=Polynomial)
    {
      z.a.resize(z.n_slots);
      for (unsigned int i=0; i<z.a.size(); i++)
        { z.a[i].sub(x.a[i],y.a[i]); }
    }
  if (z.type!=Evaluation)
    { for (unsigned int i=0; i<z.b.size(); i++)
        { z.b[i]=x.b[i]-y.b[i];
          if (z.b[i]<0)
            { z.b[i]=1; }
        }
    }
}



template<class T,class FD,class S>
void mul(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x,const Plaintext<T,FD,S>& y)
{
  if (z.Field_Data!=x.Field_Data)  { throw field_mismatch(); }
  if (z.Field_Data!=y.Field_Data)  { throw field_mismatch(); }

  if (y.type==Polynomial) { throw not_implemented(); }
  if (x.type==Polynomial) { throw not_implemented(); }
  z.type=Evaluation;

  z.allocate();
  for (unsigned int i=0; i<z.a.size(); i++)
    { z.a[i].mul(x.a[i],y.a[i]); }
}

template<class T,class FD,class S>
void sqr(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x)
{
  if (z.Field_Data!=x.Field_Data)  { throw field_mismatch(); }

  if (x.type==Polynomial) { throw not_implemented(); }
  z.type=Evaluation;

  z.allocate();
  for (unsigned int i=0; i<z.a.size(); i++)
    { z.a[i].square(x.a[i]); }
}


template<>
void Plaintext<gfp,FFT_Data,bigint>::negate()
{
  if (type!=Polynomial)
    {
      a.resize(n_slots);
      for (unsigned int i=0; i<a.size(); i++)
        { a[i].negate(); }
    }
  if (type!=Evaluation)
    { for (unsigned int i=0; i<b.size(); i++)
        { if (b[i]!=0)
            {
              b[i]-=(*Field_Data).get_prime();
              b[i].negate();
            }
	}
    }
}

template<>
void Plaintext<gfp,PPData,bigint>::negate()
{
  if (type!=Polynomial)
    {
      a.resize(n_slots);
      for (unsigned int i=0; i<a.size(); i++)
        { a[i].negate(); }
    }
  if (type!=Evaluation)
    { for (unsigned int i=0; i<b.size(); i++)
        { if (b[i]!=0)
            { b[i]=(*Field_Data).get_prime()-b[i]; }
	}
    }
}



template<>
void Plaintext<gf2n_short,P2Data,int>::negate()
{
  return;
}



template<class T,class FD,class S>
bool Plaintext<T,FD,S>::equals(const Plaintext<T,FD,S>& x) const
{
  if (Field_Data!=x.Field_Data) { return false; }
  if (type!=x.type)
    { if (type==Evaluation) { x.from_poly(); }
      else                  { from_poly(); }
    }

  if (type!=Polynomial and x.type!=Polynomial)
    {
      a.resize(n_slots);
      for (unsigned int i=0; i<a.size(); i++)
       { if (!a[i].equal(x.a[i])) { return false; } }
    }
  else
    { for (unsigned int i=0; i<b.size(); i++)
       { if (b[i]!=x.b[i]) { return false; } }
    }
  return true;
}



template <class T,class FD,class S>
void Plaintext<T,FD,S>::pack(octetStream& o) const
{
  to_poly();
  o.store((unsigned int)b.size());
  for (unsigned int i = 0; i < b.size(); i++)
    o.store(b[i]);
}

template <class T,class FD,class S>
void Plaintext<T,FD,S>::unpack(octetStream& o)
{
  type = Polynomial;
  unsigned int size;
  o.get(size);
  allocate();
  if (size != b.size())
    throw length_error("unexpected length received");
  for (unsigned int i = 0; i < b.size(); i++)
    o.get(b[i]);
}



template <>
size_t Plaintext<gfp, FFT_Data, bigint>::report_size(ReportType type)
{
  size_t res = 0;
  if (type != MINIMAL)
    res += sizeof(gfp) * a.capacity();
  for (unsigned int i = 0; i < b.size(); i++)
    res += b[i].report_size(type);
  return res;
}


template <>
size_t Plaintext<gf2n_short, P2Data, int>::report_size(ReportType type)
{
  size_t res = 0;
  if (type != MINIMAL)
    res += sizeof(gf2n_short) * a.capacity();
  res += sizeof(int) * b.size();
  return res;
}


template<class T, class FD, class S>
void Plaintext<T, FD, S>::print_evaluation(int n_elements, string desc) const
{
  cout << desc;
  for (int i = 0; i < n_elements; i++)
    cout << " " << element(i);
  cout << endl;
}



template class Plaintext<gfp,FFT_Data,bigint>;
template ostream& operator<<(ostream& s,const Plaintext<gfp,FFT_Data,bigint>& e);
template istream& operator>>(istream& s,Plaintext<gfp,FFT_Data,bigint>& e);

template void mul(Plaintext<gfp,FFT_Data,bigint>& z,const Plaintext<gfp,FFT_Data,bigint>& x,const Plaintext<gfp,FFT_Data,bigint>& y);
template void sqr(Plaintext<gfp,FFT_Data,bigint>& z,const Plaintext<gfp,FFT_Data,bigint>& x);



template class Plaintext<gfp,PPData,bigint>;
template ostream& operator<<(ostream& s,const Plaintext<gfp,PPData,bigint>& e);
template istream& operator>>(istream& s,Plaintext<gfp,PPData,bigint>& e);

template void mul(Plaintext<gfp,PPData,bigint>& z,const Plaintext<gfp,PPData,bigint>& x,const Plaintext<gfp,PPData,bigint>& y);
template void sqr(Plaintext<gfp,PPData,bigint>& z,const Plaintext<gfp,PPData,bigint>& x);



template class Plaintext<gf2n_short,P2Data,int>;
template ostream& operator<<(ostream& s,const Plaintext<gf2n_short,P2Data,int>& e);
template istream& operator>>(istream& s,Plaintext<gf2n_short,P2Data,int>& e);

template void mul(Plaintext<gf2n_short,P2Data,int>& z,const Plaintext<gf2n_short,P2Data,int>& x,const Plaintext<gf2n_short,P2Data,int>& y);
template void sqr(Plaintext<gf2n_short,P2Data,int>& z,const Plaintext<gf2n_short,P2Data,int>& x);

