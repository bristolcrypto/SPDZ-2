// (C) 2018 University of Bristol. See License.txt

#ifndef _FHE_Params
#define _FHE_Params

/* Class to hold the FHE Parameters 
 *
 * The idea is that there is a single global FHE_Params structure
 * called params; which holds all the data
 *
 * Likewise initialising params sets up all the data for other
 * classes such the global ring etc
 *
 */
#include "FHE/FFT_Data.h"
#include "FHE/DiscreteGauss.h"
#include "Tools/random.h"

class FHE_Params
{
  protected:
  // Pair of moduli in FFTData[0] and FFTData[1] for the levels
  vector<FFT_Data> FFTData;

  // Random generator for Multivariate Gaussian Distribution etc
  RandomVectors Chi;

  // Data for distributed decryption
  int sec_p;
  bigint Bval;

  public:

  FHE_Params(int n_mults = 1) : FFTData(n_mults + 1), sec_p(-1) {}

  int n_mults() const { return FFTData.size() - 1; }

  // Rely on default copy assignment/constructor (not that they should
  // ever be needed)

  void set(const Ring& R,const vector<bigint>& primes,double r=3.2,int hwt=64);
  void set_sec(int sec);

  vector<bigint> sampleGaussian(PRNG& G, int noise_boost = 1) const
     { return Chi.sample_Gauss(G, noise_boost); }
  vector<bigint> sampleHwt(PRNG& G)      const   
     { return Chi.sample_Hwt(G); }
  vector<bigint> sampleHalf(PRNG& G)     const   
     { return Chi.sample_Half(G); }
  vector<bigint> sampleUniform(PRNG& G,const bigint& Bd) const
     { return Chi.sample_Uniform(G,Bd); }

  const vector<FFT_Data>& FFTD() const { return FFTData; }

  const bigint& p0() const           { return FFTData[0].get_prime(); }
  const bigint& p1() const           { return FFTData[1].get_prime(); }
  bigint Q() const;

  int secp() const                   { return sec_p;        }
  const bigint& B() const            { return Bval;         }
  double get_R() const               { return Chi.get_R();  }
  DiscreteGauss get_DG() const       { return Chi.get_DG(); }

  int phi_m() const                  { return FFTData[0].phi_m(); }
  const Ring& get_ring()             { return FFTData[0].get_R(); }

  void pack(octetStream& o) const;
  void unpack(octetStream& o);

  bool operator!=(const FHE_Params& other) const;
};

#endif
