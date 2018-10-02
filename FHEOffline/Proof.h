// (C) 2018 University of Bristol. See License.txt

#ifndef _Proof
#define _Proof

#include <math.h>
#include <vector>
using namespace std;

#include "Math/bigint.h"
#include "FHE/Ciphertext.h"
#include "FHE/AddableVector.h"

#include "config.h"

enum SlackType
{
  NONINTERACTIVE_SPDZ1_SLACK = -1,
  INTERACTIVE_SPDZ1_SLACK = -2,
  COVERT_SPDZ2_SLACK = -3,
  ACTIVE_SPDZ2_SLACK = -4,
};

class Proof
{
  Proof();   // Private to avoid default 

  public:

  typedef AddableVector< Int_Random_Coins > Randomness;

  class Preimages
  {
    bigint m_tmp;
    AddableVector<bigint> r_tmp;

  public:
    Preimages(int size, const FHE_PK& pk, const bigint& p, int n_players);
    AddableMatrix<bigint> m;
    Randomness r;
    void add(octetStream& os);
    void pack(octetStream& os);
    void unpack(octetStream& os);
    void check_sizes();
    size_t report_size(ReportType type) { return m.report_size(type) + r.report_size(type); }
  };

  unsigned int sec;
  bigint tau,rho;

  unsigned int phim;
  int B_plain_length, B_rand_length;
  bigint plain_check, rand_check;
  unsigned int V;

  const FHE_PK* pk;

  int n_proofs;

  static double dist;

  protected:
    Proof(int sc, const bigint& Tau, const bigint& Rho, const FHE_PK& pk,
            int n_proofs = 1) :
              B_plain_length(0), B_rand_length(0), pk(&pk), n_proofs(n_proofs)
    { sec=sc;
      tau=Tau; rho=Rho;

      phim=(pk.get_params()).phi_m();
      V=2*sec-1;
    }

  Proof(int sec, const FHE_PK& pk, int n_proofs = 1) :
      Proof(sec, pk.p() / 2, 2 * 3.2 * sqrt(pk.get_params().phi_m()), pk,
          n_proofs) {}

  public:
  static bigint slack(int slack, int sec, int phim);

  void get_challenge(vector<int>& e, const octetStream& ciphertexts) const;
  template <class T>
  bool check_bounds(T& z, AddableMatrix<bigint>& t, int i) const;
};

class NonInteractiveProof : public Proof
{
public:
  bigint static slack(int sec, int phim)
  { return bigint(phim * sec * sec) << (sec / 2 + 8); }

  NonInteractiveProof(int sec, const FHE_PK& pk,
      int extra_slack) :
      Proof(sec, pk, 1)
  {
    bigint B;
    B=128*sec*sec;
    B <<= extra_slack;
    B_plain_length = numBits(B*phim*tau);
    B_rand_length = numBits(B*3*phim*rho);
    plain_check = (bigint(1) << B_plain_length) - sec * tau;
    rand_check = (bigint(1) << B_rand_length) - sec * rho;
  }
};

class InteractiveProof : public Proof
{
public:
  bigint static slack(int sec, int phim)
  { (void)phim; return pow(2, 1.5 * sec + 1); }

  InteractiveProof(int sec, const FHE_PK& pk,
      int n_proofs = 1) :
      Proof(sec, pk, n_proofs)
  {
    bigint B;
    // using mu = 1
    B = bigint(1) << (sec - 1);
    B_plain_length = numBits(B * tau);
    B_rand_length = numBits(B * rho);
    // leeway for completeness
    plain_check = (bigint(2) << B_plain_length);
    rand_check = (bigint(2) << B_rand_length);
  }
};

#endif
