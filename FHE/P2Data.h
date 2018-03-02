// (C) 2018 University of Bristol. See License.txt

#ifndef _P2Data
#define _P2Data

/* This class defines all the data needed for representing
 * plaintexts modulo 2
 */

#include "FHE/Ring.h"
#include "FHE/Matrix.h"
#include "Math/gf2n.h"

class P2Data
{
  int slots;
  // Data for the forward mapping  (phi_m by (slots*gf2n:deg))
  imatrix A;
  // Data for the backward mapping (phi_m by phi_m)
  imatrix Ai;

  public:
  typedef gf2n_short T;
  typedef int S;

  int num_slots() const { return slots; }
  int degree() const { return A.size() ? A[0].size() : 0; }
  int phi_m() const { return A.size(); }

  void check_dimensions() const;

  // Despite only dealing with bits, we still use bigint's so
  // we can easily dovetail into the FHE code
  void forward(vector<int>& ans,const vector<gf2n_short>& a) const;
  void backward(vector<gf2n_short>& ans,const vector<int>& a) const;

  int get_prime() const { return 2; }

  bool operator!=(const P2Data& other) const;

  // no op because we require field to be initalized first
  void init_field() const {}

  void pack(octetStream& o) const;
  void unpack(octetStream& o);

  void load(const Ring& Rg);
  void store(const Ring& Rg) const;

  void load_or_generate(const Ring& Rg);

  friend ostream& operator<<(ostream& s,const P2Data& P2D);
  friend istream& operator>>(istream& s,P2Data& P2D);

  friend void init(P2Data& P2D,const Ring& Rg);
};

#endif

