// (C) 2018 University of Bristol. See License.txt

#ifndef _Rq_Element
#define _Rq_Element

/* An Rq Element is something held modulo Q_0 = p0 or Q_1 = p0*p1
 *
 * The level is the value of Q_level which is being used.
 * Elements can be held in either representation and one can switch
 * representations at will.
 *   - Although in the evaluation we do not multiply at level 1
 *     we do need to multiply at level 1 for KeyGen and Encryption.
 *
 * Usually we keep level 0 in evaluation and level 1 in polynomial
 * representation though
 */

#include "FHE/Ring_Element.h"
#include "FHE/FHE_Params.h"
#include "FHE/tools.h"
#include "FHE/Generator.h"
#include <vector>

// Forward declare the friend functions
class Rq_Element;
void add(Rq_Element& ans,const Rq_Element& a,const Rq_Element& b);
void sub(Rq_Element& ans,const Rq_Element& a,const Rq_Element& b);
void mul(Rq_Element& ans,const Rq_Element& a,const Rq_Element& b);
void mul(Rq_Element& ans,const Rq_Element& a,const bigint& b);
ostream& operator<<(ostream& s,const Rq_Element& a);
istream& operator>>(istream& s, Rq_Element& a);

class Rq_Element
{
protected:
  vector<Ring_Element> a;
  int lev;

  public:
  
  int n_mults() const { return a.size() - 1; }

  void change_rep(RepType r);
  void change_rep(RepType r0,RepType r1);

  void set_data(const vector<FFT_Data>& prd);
  void assign_zero(const vector<FFT_Data>& prd);
  void assign_zero(); 
  void assign_one(); 
  void assign(const Rq_Element& e);
  void partial_assign(const Rq_Element& e);

  // Must be careful not to call by mistake
  Rq_Element(RepType r0=evaluation,RepType r1=polynomial) :
    a({r0, r1}), lev(n_mults()) {}

  // Pass in a pair of FFT_Data as a vector
  Rq_Element(const vector<FFT_Data>& prd, RepType r0 = evaluation,
      RepType r1 = polynomial);

  Rq_Element(const FHE_Params& params, RepType r0, RepType r1) :
      Rq_Element(params.FFTD(), r0, r1) {}

  Rq_Element(const FHE_Params& params) :
      Rq_Element(params.FFTD()) {}

  Rq_Element(const Ring_Element& b0,const Ring_Element& b1) :
    a({b0, b1}), lev(n_mults()) {}

  // Destructor
  ~Rq_Element()
     { ; }

  // Copy Assignment
  Rq_Element& operator=(const Rq_Element& e)
    { if (this!=&e) { assign(e); }
      return *this;
    }

  const Ring_Element& get(int i) const { return a[i]; }

  /* Functional Operators */
  void negate();
  friend void add(Rq_Element& ans,const Rq_Element& a,const Rq_Element& b);
  friend void sub(Rq_Element& ans,const Rq_Element& a,const Rq_Element& b);
  friend void mul(Rq_Element& ans,const Rq_Element& a,const Rq_Element& b);
  friend void mul(Rq_Element& ans,const Rq_Element& a,const bigint& b);

  Rq_Element& operator+=(const Rq_Element& other) { add(*this, *this, other); return *this; }

  Rq_Element operator+(const Rq_Element& b) const { Rq_Element res(*this); add(res, *this, b); return res; }
  Rq_Element operator-(const Rq_Element& b) const { Rq_Element res(*this); sub(res, *this, b); return res; }
  template <class T>
  Rq_Element operator*(const T& b) const { Rq_Element res(*this); mul(res, *this, b); return res; }

  // Multiply something by p1 and make level 1
  void mul_by_p1();

  void randomize(PRNG& G,int lev=-1);

  // Scale from level 1 to level 0, if at level 0 do nothing
  void Scale(const bigint& p);

  bool equals(const Rq_Element& a) const;
  bool operator!=(const Rq_Element& a) const { return !equals(a); }

  int level() const { return lev; }
  void lower_level() { if (lev==1) { lev=0; }  }
  // raise_level boosts a level 0 to a level 1 (or does nothing if level =1)
  void raise_level();
  void check_level() const;
  void set_level(int level) { lev = (level == -1 ? n_mults() : level); }
  void partial_assign(const Rq_Element& a, const Rq_Element& b);

  // Converting to and from a vector of bigint's Again I/O is in poly rep
  void from_vec(const vector<bigint>& v,int level=-1);
  void from_vec(const vector<int>& v,int level=-1);
  vector<bigint>  to_vec_bigint() const;
  void to_vec_bigint(vector<bigint>& v) const;

  ConversionIterator get_iterator();
  template <class T>
  void from(const Generator<T>& generator, int level=-1);

  bigint infinity_norm() const;

  bigint get_prime(int i) const
    { return a[i].get_prime(); }

  bigint get_modulus() const
    { bigint ans=1;
      for(int i=0; i<=lev; ++i) ans*=a[i].get_prime();
      return ans;
    }

  friend ostream& operator<<(ostream& s,const Rq_Element& a);
  friend istream& operator>>(istream& s, Rq_Element& a);

  /* Pack and unpack into an octetStream 
   *   For unpack we assume the prData for a0 and a1 has been assigned 
   *   correctly already
   */
  void pack(octetStream& o) const;
  void unpack(octetStream& o);

  void output(ostream& s) const;
  void input(istream& s);

  void check(const FHE_Params& params) const;

  size_t report_size(ReportType type) const;

  void print_first_non_zero() const;
};
   
template<int L>
inline void mul(Rq_Element& ans,const bigint& a,const Rq_Element& b)
{ mul(ans,b,a); }

#endif

