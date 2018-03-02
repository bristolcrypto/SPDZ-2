// (C) 2018 University of Bristol. See License.txt

#ifndef _PPData
#define _PPData

#include "Math/modp.h"
#include "Math/Zp_Data.h"
#include "Math/gfp.h"
#include "FHE/Ring.h"

/* Class for holding modular arithmetic data wrt the ring 
 *
 * It also holds the ring
 */

class PPData
{
  public:
  typedef gf2n_short T;
  typedef bigint S;

  Ring    R;
  Zp_Data prData;

  modp root;        // m'th Root of Unity mod pr 
  
  void init(const Ring& Rg,const Zp_Data& PrD);

  void assign(const PPData& PPD);

  PPData() { ; }
  PPData(const PPData& PPD)
    { assign(PPD); }
  PPData(const Ring& Rg,const Zp_Data& PrD)
    { init(Rg,PrD); }
  PPData& operator=(const PPData& PPD)
    { if (this!=&PPD) { assign(PPD); }
      return *this;
    }

  const Zp_Data& get_prD() const   { return prData; }
  const bigint&  get_prime() const { return prData.pr; }
  int phi_m() const                { return R.phi_m(); }
  int m()     const                { return R.m();   }
  int num_slots() const { return R.phi_m(); }
  

  int p(int i)      const        { return R.p(i);     }
  int p_inv(int i)  const        { return R.p_inv(i); }
  const vector<int>& Phi() const { return R.Phi();    }

  friend ostream& operator<<(ostream& s,const PPData& PPD); 
  friend istream& operator>>(istream& s,PPData& PPD); 

  // Convert input vector from poly to evaluation representation
  //   - Uses naive method and not FFT, we only use this rarely in any case
  void to_eval(vector<modp>& elem) const;
  void from_eval(vector<modp>& elem) const;

  // Following are used to iteratively get slots, as we use PPData when
  // we do not have an efficient FFT algorithm
  gfp thetaPow,theta;
  int pow;
  void reset_iteration();
  void next_iteration(); 
  gfp get_evaluation(const vector<bigint>& mess) const;
  
};

#endif

