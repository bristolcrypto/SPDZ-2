// (C) 2018 University of Bristol. See License.txt

#ifndef _Plaintext
#define _Plaintext

/* 
 * Defines plaintext either as a vector of field elements (gfp or gf2n_short)
 * or as a vector of int (for gf2n_short) or bigints (for gfp)
 *
 * The first format is the actual data we want, the second format
 * is the data we use to encrypt/decrypt. Passing between the two
 * depends on whether we have a gfp or a gf2n_short base type.
 *   - The former uses the FFT (in the case of special m/p values
 *     and using FFT_Data), or naive methods (for general m/p values
 *     and using PPData), the second uses a precomputed linear
 *     map
 *
 * We hold data however in vector format, as this is easier to 
 * deal with
 */

#include "Math/bigint.h"
#include "FHE/FFT_Data.h"
#include "FHE/P2Data.h"
#include "FHE/PPData.h"
#include "Math/gfp.h"
#include "Math/gf2n.h"
#include "FHE/Generator.h"

#include <vector>
using namespace std;

// Forward declaration as apparently this is needed for friends in templates
template<class T,class FD,class S> class Plaintext;
template<class T,class FD,class S> ostream& operator<<(ostream& s,const Plaintext<T,FD,S>& e);
template<class T,class FD,class S> istream& operator>>(istream& s, Plaintext<T,FD,S>& e);
template<class T,class FD,class S> void add(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x,const Plaintext<T,FD,S>& y);
template<class T,class FD,class S> void sub(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x,const Plaintext<T,FD,S>& y);
template<class T,class FD,class S> void mul(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x,const Plaintext<T,FD,S>& y);
template<class T,class FD,class S> void sqr(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x);

enum condition { Full, Diagonal, Bits };

enum PT_Type   { Polynomial, Evaluation, Both }; 

template<class T,class FD,class S>
class Plaintext
{
  int n_slots;
  int degree;

  mutable vector<T> a;  // The thing in evaluation/FFT form
  mutable vector<S> b;  // Now in polynomial form

  mutable PT_Type type;

  /* We keep pointers to the basic data here 
   *   - FD is of type FFT_Data or PPData if T is of type gfp
   *   - FD is of type P2Data if T is of type gf2n_short
   */

  const FD   *Field_Data;

  void set_sizes();

  public:
  
  const FD& get_field() const { return *Field_Data; }
  unsigned int num_slots() const { return n_slots; }

  void assign(const Plaintext<T,FD,S>& p)
    { Field_Data=p.Field_Data;
      a=p.a; b=p.b; type=p.type;
      n_slots = p.n_slots;
      degree = p.degree;
    }

  Plaintext(const FD& FieldD, PT_Type type = Polynomial)
  { Field_Data=&FieldD; set_sizes(); allocate(type); }

  Plaintext(const Plaintext<T,FD,S>& p)   { assign(p); }
  ~Plaintext() { ; }
  Plaintext& operator=(const Plaintext<T,FD,S>& p)
    { if (this!=&p) { assign(p); }
      return *this;
    }

  void allocate(PT_Type type) const;
  void allocate() const { allocate(type); }
  void allocate_slots(const bigint& value);
  int get_min_alloc();

  // Access evaluation representation
  T element(int i) const
    {  if (type==Polynomial)  
         { from_poly(); }
       return a[i]; 
    }
  void set_element(int i,const T& e)
    { if (type==Polynomial)
        { throw not_implemented(); }
      a.resize(n_slots);
      a[i]=e;
      type=Evaluation;
    }

  // Access poly representation
  const S& coeff(int i) const
    {  if (type!=Polynomial)
         { to_poly(); }
       return b[i];
    }

  void set_coeff(int i,const S& e)
    {
      to_poly();
      type=Polynomial;
      b[i]=e;
    }

  const S& operator[](int i) const { return coeff(i); }

  // Assumes v is of the correct length (phi_m) for the given ring
  // and is already reduced modulo the correct prime etc
  void set_poly(const vector<S>& v)
    { type=Polynomial; b=v; }
  const vector<S>& get_poly() const
    { if (type==Evaluation) { throw rep_mismatch(); }
      return b;
    }

  Iterator<S> get_iterator() const { to_poly(); return b; }

  // This sets a poly from a vector of bigint's which needs centering
  // modulo mod, before assigning (used in decryption)
  //    vv[i] is already assumed reduced modulo mod though but in 
  //    range [0,...,mod)
  void set_poly_mod(const vector<bigint>& vv,const bigint& mod);
  void set_poly_mod(const Generator<bigint>& generator, const bigint& mod);

  // Converts between Evaluation,Polynomial and Both representations
  //    Marked as const because does not change value, only changes the
  //    internal representation
  void from_poly() const;
  void to_poly() const;

  void randomize(PRNG& G,condition cond=Full);
  void randomize(PRNG& G, bigint B, bool Diag=false, bool binary=false, PT_Type type=Polynomial);
  void randomize(PRNG& G, int n_bits, bool Diag=false, bool binary=false, PT_Type type=Polynomial);

  void assign_zero(PT_Type t = Evaluation);
  void assign_one(PT_Type t = Evaluation);

  friend void add<>(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x,const Plaintext<T,FD,S>& y);
  friend void sub<>(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x,const Plaintext<T,FD,S>& y);
  friend void mul<>(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x,const Plaintext<T,FD,S>& y);
  friend void sqr<>(Plaintext<T,FD,S>& z,const Plaintext<T,FD,S>& x);

  Plaintext<T,FD,S> operator+(const Plaintext<T,FD,S>& x) const
  { Plaintext<T,FD,S> res(*Field_Data); add(res, *this, x); return res; }
  Plaintext<T,FD,S> operator-(const Plaintext<T,FD,S>& x) const
  { Plaintext<T,FD,S> res(*Field_Data); sub(res, *this, x); return res; }

  void mul(const Plaintext<T,FD,S>& x, const Plaintext<T,FD,S>& y)
  { x.from_poly(); y.from_poly(); ::mul(*this, x, y); }

  Plaintext<T,FD,S> operator*(const Plaintext<T,FD,S>& x)
  { Plaintext<T,FD,S> res(*Field_Data); res.mul(*this, x); return res; }

  Plaintext<T,FD,S>& operator+=(const Plaintext<T,FD,S>& y);
  Plaintext<T,FD,S>& operator-=(const Plaintext<T,FD,S>& y)
  { to_poly(); y.to_poly(); ::sub(*this, *this, y); return *this; }

  void negate();

  bool equals(const Plaintext<T,FD,S>& x) const;
  bool operator!=(const Plaintext<T,FD,S>& x) { return !equals(x); }

  bool is_diagonal() const { throw not_implemented(); }
  bool is_binary() const { throw not_implemented(); }

  friend ostream& operator<< <>(ostream& s,const Plaintext<T,FD,S>& e);
  friend istream& operator>> <>(istream& s,Plaintext<T,FD,S>& e);

  /* Pack and unpack into an octetStream 
   *   For unpack we assume the FFTD has been assigned correctly already
   */
  void pack(octetStream& o) const;
  void unpack(octetStream& o);

  size_t report_size(ReportType type);

  void print_evaluation(int n_elements, string desc = "") const;
};

template <class FD>
using Plaintext_ = Plaintext<typename FD::T, FD, typename FD::S>;

#endif
