// (C) 2018 University of Bristol. See License.txt


#include "Share.h"
//#include "Tools/random.h"
#include "Math/gfp.h"
#include "Math/gf2n.h"
#include "Math/operators.h"


template<class T>
Share<T>::Share(const T& aa, int my_num, const T& alphai)
{
    if (my_num == 0)
        a = aa;
    else
        a.assign_zero();
    mac = aa * alphai;
}


template<class T>
void Share<T>::mul_by_bit(const Share<T>& S,const T& aa)
{
  a.mul(S.a,aa);
  mac.mul(S.mac,aa);
}

template<>
void Share<gf2n>::mul_by_bit(const Share<gf2n>& S, const gf2n& aa)
{
  a.mul_by_bit(S.a,aa);
  mac.mul_by_bit(S.mac,aa);
}

template<class T>
void Share<T>::add(const Share<T>& S,const T& aa,bool playerone,const T& alphai)
{
  if (playerone) 
     { a.add(S.a,aa); }
  else           
     { a=S.a;   }

  T tmp;
  tmp.mul(alphai,aa);
  mac.add(S.mac,tmp);
}



template<class T>
void Share<T>::sub(const Share<T>& S,const T& aa,bool playerone,const T& alphai)
{
  if (playerone) 
    { a.sub(S.a,aa); }
  else           
    { a=S.a;   }

  T tmp;
  tmp.mul(alphai,aa);
  mac.sub(S.mac,tmp);
}



template<class T>
void Share<T>::sub(const T& aa,const Share<T>& S,bool playerone,const T& alphai)
{
  if (playerone) 
    { a.sub(aa,S.a); }
  else           
    { a=S.a;
      a.negate(); 
    }

  T tmp;
  tmp.mul(alphai,aa);
  mac.sub(tmp,S.mac);
}



template<class T>
void Share<T>::sub(const Share<T>& S1,const Share<T>& S2)
{
  a.sub(S1.a,S2.a);
  mac.sub(S1.mac,S2.mac);
}



template<class T>
T combine(const vector< Share<T> >& S)
{
  T ans=S[0].a;
  for (unsigned int i=1; i<S.size(); i++) 
    { ans.add(ans,S[i].a); }
  return ans;
}




template<class T>
inline void Share<T>::pack(octetStream& os) const
{
  a.pack(os);
  mac.pack(os);
}

template<class T>
inline void Share<T>::unpack(octetStream& os)
{
  a.unpack(os);
  mac.unpack(os);
}


template<class T>
bool check_macs(const vector< Share<T> >& S,const T& key)
{
  T val=combine(S);

  // Now check the MAC is valid
  val.mul(val,key);
  for (unsigned i=0; i<S.size(); i++)
    { val.sub(val,S[i].mac); }
  if (!val.is_zero()) { return false; }
  return true;
}

template class Share<gf2n>;
template class Share<gfp>;
template gf2n combine(const vector< Share<gf2n> >& S);
template gfp combine(const vector< Share<gfp> >& S);
template bool check_macs(const vector< Share<gf2n> >& S,const gf2n& key);
template bool check_macs(const vector< Share<gfp> >& S,const gfp& key);

#ifdef USE_GF2N_LONG
template class Share<gf2n_short>;
template gf2n_short combine(const vector< Share<gf2n_short> >& S);
template bool check_macs(const vector< Share<gf2n_short> >& S,const gf2n_short& key);
#endif
