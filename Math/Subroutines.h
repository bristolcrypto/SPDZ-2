// (C) 2018 University of Bristol. See License.txt

#ifndef _Subroutines
#define _Subroutines


#include "Math/Zp_Data.h"

void Subs(modp& ans,const vector<int>& poly,const modp& x,const Zp_Data& ZpD);


/* Find an m'th primitive root moduli the current prime
 *   This is deterministic so all players have the same root of unity
 *   poly is Phi_m(X)
 */
modp Find_Primitive_Root_m(int m,const vector<int>& poly,const Zp_Data& ZpD);


/* Find a (2m)'th primitive root moduli the current prime
 *   This is deterministic so all players have the same root of unity
 *   poly is Phi_m(X)
 */
modp Find_Primitive_Root_2m(int m,const vector<int>& poly,const Zp_Data& ZpD);


/* Find an mth primitive root moduli the current prime
 *   This is deterministic so all players have the same root of unity
 * This assumes m is a power of two and so the cyclotomic polynomial
 * is  F=X^{m/2}+1
 */
modp Find_Primitive_Root_2power(int m,const Zp_Data& ZpD);

#endif
