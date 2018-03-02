// (C) 2018 University of Bristol. See License.txt

#ifndef _Ring
#define _Ring

/* Class to define the basic ring we are working with
 * and any associated data
 */

#include <vector>
#include <iostream>
using namespace std;

#include "Tools/octetStream.h"

class Ring
{
  int mm,phim;
  vector<int> pi;
  vector<int> pi_inv;
  vector<int> poly;
  
  public:


  Ring() : mm(0), phim(0) { ; }
  ~Ring()     { ; }

  // Rely on default copy assignment/constructor
  
  int phi_m() const { return phim; }
  int m()     const { return mm;   }

  int p(int i)      const { return pi.at(i);     }
  int p_inv(int i)  const { return pi_inv.at(i); }
  const vector<int>& Phi() const { return poly;         }

  friend ostream& operator<<(ostream& s,const Ring& R);
  friend istream& operator>>(istream& s,Ring& R);

  friend void init(Ring& Rg,int m);

  void pack(octetStream& o) const;
  void unpack(octetStream& o);

  bool operator!=(const Ring& other) const;
};

#endif

