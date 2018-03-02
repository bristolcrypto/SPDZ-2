// (C) 2018 University of Bristol. See License.txt

#ifndef _Ciphertext
#define _Ciphertext

#include "FHE/FHE_Keys.h"
#include "FHE/Random_Coins.h"
#include "FHE/Plaintext.h"

class FHE_PK;
class Ciphertext;

// Forward declare the friend functions
template<class T,class FD,class S> void mul(Ciphertext& ans,const Plaintext<T,FD,S>& a,const Ciphertext& c);
template<class T,class FD,class S> void mul(Ciphertext& ans,const Ciphertext& c,const Plaintext<T,FD,S>& a);

void add(Ciphertext& ans,const Ciphertext& c0,const Ciphertext& c1);
void mul(Ciphertext& ans,const Ciphertext& c0,const Ciphertext& c1,const FHE_PK& pk);

class Ciphertext
{
  Rq_Element cc0,cc1;
  const FHE_Params *params;
  // identifier for debugging
  word pk_id;

  public:
  static string type_string() { return "ciphertext"; }
  static int t() { return 0; }
  static int size() { return 0; }

  const FHE_Params& get_params() const { return *params; }

  Ciphertext(const FHE_Params& p)
    : cc0(p.FFTD(),evaluation,evaluation),
      cc1(p.FFTD(),evaluation,evaluation), pk_id(0)  { params=&p; }

  Ciphertext(const FHE_PK &pk);

  ~Ciphertext() {  ; }

  // Rely on default copy assignment/constructor
  
  void set(const Rq_Element& a0, const Rq_Element& a1, word pk_id)
    { cc0=a0; cc1=a1; this->pk_id = pk_id; }
  void set(const Rq_Element& a0, const Rq_Element& a1, const FHE_PK& pk);

  const Rq_Element& c0() const { return cc0; }
  const Rq_Element& c1() const { return cc1; }
  
  void assign_zero() { cc0.assign_zero(); cc1.assign_zero(); pk_id = 0; }

  // Assumes IO already knows what params, and have set them already
  friend ostream& operator<<(ostream& s,const Ciphertext& c)
    { s << "[ " << c.pk_id << " " << c.cc0 << " , " << c.cc1 << "]"; return s; }
  friend istream& operator>>(istream& s,Ciphertext& c);

  // Scale down an element from level 1 to level 0, if at level 0 do nothing
  void Scale(const bigint& p)    { cc0.Scale(p); cc1.Scale(p); }

  // Throws error if ans,c0,c1 etc have different params settings
  //   - Thus programmer needs to ensure this rather than this being done
  //     automatically. This saves some time in space initialization
  friend void add(Ciphertext& ans,const Ciphertext& c0,const Ciphertext& c1);
  friend void sub(Ciphertext& ans,const Ciphertext& c0,const Ciphertext& c1);
  friend void mul(Ciphertext& ans,const Ciphertext& c0,const Ciphertext& c1,const FHE_PK& pk);
  template<class T,class FD,class S> friend void mul(Ciphertext& ans,const Plaintext<T,FD,S>& a,const Ciphertext& c);
  template<class T,class FD,class S> friend void mul(Ciphertext& ans,const Ciphertext& c,const Plaintext<T,FD,S>& a)
     { ::mul(ans,a,c); }

  void mul(const Ciphertext& c, const Rq_Element& a);

  template<class FD>
  void mul(const Ciphertext& c, const Plaintext_<FD>& a) { ::mul(*this, c, a); }

  bool operator==(const Ciphertext& c) { return pk_id == c.pk_id && cc0.equals(c.cc0) && cc1.equals(c.cc1); }
  bool operator!=(const Ciphertext& c) { return !(*this == c); }

  Ciphertext operator+(const Ciphertext& other) const
  { Ciphertext res(*params); ::add(res, *this, other); return res; }

  template <class FD>
  Ciphertext operator*(const Plaintext_<FD>& other) const
  { Ciphertext res(*params); ::mul(res, *this, other); return res; }

  Ciphertext& operator+=(const Ciphertext& other) { ::add(*this, *this, other); return *this; }

  template <class FD>
  Ciphertext& operator*=(const Plaintext_<FD>& other) { ::mul(*this, *this, other); return *this; }

  Ciphertext mul(const Ciphertext& x, const FHE_PK& pk) const
  { Ciphertext res(*params); ::mul(res, *this, x, pk); return res; }

  int level() const { return cc0.level(); }

  // pack/unpack (like IO) also assume params are known and already set 
  // correctly
  void pack(octetStream& o) const
    { cc0.pack(o); cc1.pack(o); o.store(pk_id); }
  void unpack(octetStream& o) 
    { cc0.unpack(o); cc1.unpack(o); o.get(pk_id); }

  void output(ostream& s) const
    { cc0.output(s); cc1.output(s); s.write((char*)&pk_id, sizeof(pk_id)); }
  void input(istream& s)
    { cc0.input(s); cc1.input(s); s.read((char*)&pk_id, sizeof(pk_id)); }

  template <int t>
  void add(octetStream& os);

  size_t report_size(ReportType type) const { return cc0.report_size(type) + cc1.report_size(type); }
};

#endif
