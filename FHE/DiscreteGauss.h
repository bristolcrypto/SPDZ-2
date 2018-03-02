// (C) 2018 University of Bristol. See License.txt

#ifndef _DiscreteGauss
#define _DiscreteGauss

/* Class to sample from a Discrete Gauss distribution of 
   standard deviation R
*/

#include <FHE/Generator.h>
#include "Math/modp.h"
#include "Tools/random.h"
#include <vector>

/* Uses the Ratio method as opposed to the Box-Muller method
 * as the Ratio method is thread safe, but it is 50% slower
 */

class DiscreteGauss
{
  double R;         // Standard deviation
  double e;         // Precomputed exp(1)
  double e1;        // Precomputed exp(0.25)
  double e2;        // Precomputed exp(-1.35)

  public:

  void set(double R);

  void pack(octetStream& o) const { o.serialize(R); }
  void unpack(octetStream& o) { o.unserialize(R); }

  DiscreteGauss() { set(0); }
  DiscreteGauss(double R) { set(R); }
  ~DiscreteGauss()        { ; }

  // Rely on default copy constructor/assignment
  
  int sample(PRNG& G, int stretch = 1) const;
  double get_R() const { return R; }

  bool operator!=(const DiscreteGauss& other) const;
};


/* Sample from integer lattice of dimension n 
 * with standard deviation R
 */
class RandomVectors
{
  int n,h;
  DiscreteGauss DG;  // This generates the main distribution

  public:

  void set(int nn,int hh,double R);  // R is input STANDARD DEVIATION

  void pack(octetStream& o) const { o.store(n); o.store(h); DG.pack(o); }
  void unpack(octetStream& o) { o.get(n); o.get(h); DG.unpack(o); }

  RandomVectors() { ; }
  RandomVectors(int nn,int hh,double R)    { set(nn,hh,R);  }
  ~RandomVectors()        { ; }

  // Rely on default copy constructor/assignment

  double get_R() const { return DG.get_R(); }
  DiscreteGauss get_DG() const { return DG; }
  
  // Sample from Discrete Gauss distribution
  vector<bigint> sample_Gauss(PRNG& G, int stretch = 1) const;

  // Next samples from Hwt distribution unless hwt>n/2 in which 
  // case it uses Gauss
  vector<bigint> sample_Hwt(PRNG& G) const;

  // Sample from {-1,0,1} with Pr(-1)=Pr(1)=1/4 and Pr(0)=1/2
  vector<bigint> sample_Half(PRNG& G) const;

  // Sample from (-B,0,B) with uniform prob
  vector<bigint> sample_Uniform(PRNG& G,const bigint& B) const;

  bool operator!=(const RandomVectors& other) const;
};

class RandomGenerator : public Generator<bigint>
{
protected:
  mutable PRNG G;

public:
  RandomGenerator(PRNG& G) { this->G.SetSeed(G); }
};

class UniformGenerator : public RandomGenerator
{
  int n_bits;
  bool positive;

public:
  UniformGenerator(PRNG& G, int n_bits, bool positive = true) :
    RandomGenerator(G), n_bits(n_bits), positive(positive) {}
  Generator* clone() const { return new UniformGenerator(*this); }
  void get(bigint& x) const  { G.get_bigint(x, n_bits, positive); }
};

class GaussianGenerator : public RandomGenerator
{
  DiscreteGauss DG;
  int stretch;

public:
  GaussianGenerator(const DiscreteGauss& DG, PRNG& G, int stretch = 1) :
    RandomGenerator(G), DG(DG), stretch(stretch) {}
  Generator* clone() const { return new GaussianGenerator(*this); }
  void get(bigint& x) const { mpz_set_si(x.get_mpz_t(), DG.sample(G, stretch)); }
};

int sample_half(PRNG& G);

class HalfGenerator : public RandomGenerator
{
public:
  HalfGenerator(PRNG& G) :
    RandomGenerator(G) {}
  Generator* clone() const { return new HalfGenerator(*this); }
  void get(bigint& x) const { mpz_set_si(x.get_mpz_t(), sample_half(G)); }
};

#endif
