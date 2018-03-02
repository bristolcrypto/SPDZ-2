// (C) 2018 University of Bristol. See License.txt


#ifndef _Share
#define _Share

/* Class for holding a share of either a T or gfp element */ 

#include <vector>
#include <iostream>
using namespace std;

#include "Math/gfp.h"
#include "Math/gf2n.h"

// Forward declaration as apparently this is needed for friends in templates
template<class T> class Share;
template<class T> T combine(const vector< Share<T> >& S);
template<class T> bool check_macs(const vector< Share<T> >& S,const T& key);


template<class T>
class Share
{
   T a;        // The share
   T mac;      // Shares of the mac

   public:

   typedef T value_type;

   static int size()
     { return 2 * T::size(); }

   static string type_string()
     { return T::type_string(); }

   void assign(const Share<T>& S)
     { a=S.a; mac=S.mac; }
   void assign(const char* buffer)
     { a.assign(buffer); mac.assign(buffer + T::size()); }
   void assign_zero()
     { a.assign_zero(); 
       mac.assign_zero(); 
     }

   Share()                  { assign_zero(); }
   Share(const Share<T>& S) { assign(S); }
   Share(const T& aa, int my_num, const T& alphai);
   ~Share()                 { ; }
   Share& operator=(const Share<T>& S)
     { if (this!=&S) { assign(S); }
       return *this;
     }

   const T& get_share() const          { return a; }
   const T& get_mac() const            { return mac; }
   void set_share(const T& aa)  { a=aa; }
   void set_mac(const T& aa)    { mac=aa; }

   /* Arithmetic Routines */
   void mul(const Share<T>& S,const T& aa);
   void mul_by_bit(const Share<T>& S,const T& aa);
   void add(const Share<T>& S,const T& aa,bool playerone,const T& alphai);
   void negate() { a.negate(); mac.negate(); }
   void sub(const Share<T>& S,const T& aa,bool playerone,const T& alphai);
   void sub(const T& aa,const Share<T>& S,bool playerone,const T& alphai);
   void add(const Share<T>& S1,const Share<T>& S2);
   void sub(const Share<T>& S1,const Share<T>& S2);
   void add(const Share<T>& S1) { add(*this,S1); }

   Share<T> operator+(const Share<T>& x) const
   { Share<T> res; res.add(*this, x); return res; }
   template <class U>
   Share<T> operator*(const U& x) const
   { Share<T> res; res.mul(*this, x); return res; }

   Share<T>& operator+=(const Share<T>& x) { add(x); return *this; }
   template <class U>
   Share<T>& operator*=(const U& x) { mul(*this, x); return *this; }

   Share<T> operator<<(int i) { return this->operator*(T(1) << i); }
   Share<T>& operator<<=(int i) { return *this = *this << i; }

   // Input and output from a stream
   //  - Can do in human or machine only format (later should be faster)
   void output(ostream& s,bool human) const
     { a.output(s,human);     if (human) { s << " "; }
       mac.output(s,human);
     }
   void input(istream& s,bool human)
     { a.input(s,human);
       mac.input(s,human);
     }

   friend ostream& operator<<(ostream& s, const Share<T>& x) { x.output(s, true); return s; }

   void pack(octetStream& os) const;
   void unpack(octetStream& os);

    /* Takes a vector of shares, one from each player and
     * determines the shared value
     *   - i.e. Partially open the shares
     */
   friend T combine<T>(const vector< Share<T> >& S);

   /* Given a set of shares, one from each player and
    * the global key, determines if the sharing is valid
    *   - Mainly for test purposes
    */
   friend bool check_macs<T>(const vector< Share<T> >& S,const T& key);
};

// specialized mul by bit for gf2n
template <>
void Share<gf2n>::mul_by_bit(const Share<gf2n>& S,const gf2n& aa);

template <class T>
Share<T> operator*(const T& y, const Share<T>& x) { Share<T> res; res.mul(x, y); return res; }

template<class T>
inline void Share<T>::add(const Share<T>& S1,const Share<T>& S2)
{
  a.add(S1.a,S2.a);
  mac.add(S1.mac,S2.mac);
}

template<class T>
inline void Share<T>::mul(const Share<T>& S,const T& aa)
{
  a.mul(S.a,aa);
  mac.mul(S.mac,aa);
}

#endif
