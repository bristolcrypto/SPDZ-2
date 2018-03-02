// (C) 2018 University of Bristol. See License.txt

/*
 * Proof.cpp
 *
 */

#include "Proof.h"
#include "FHEOffline/EncCommit.h"


double Proof::dist = 0;

bigint Proof::slack(int slack, int sec, int phim)
{
  switch (slack)
  {
    case NONINTERACTIVE_SPDZ1_SLACK:
      cout << "Computing slack for non-interactive SPDZ1 proof" << endl;
      return NonInteractiveProof::slack(sec, phim);
    case INTERACTIVE_SPDZ1_SLACK:
      cout << "Computing slack for interactive SPDZ1 proof" << endl;
      return InteractiveProof::slack(sec, phim);
    case COVERT_SPDZ2_SLACK:
      cout << "No slack for covert SPDZ2 proof" << endl;
      return 0;
    case ACTIVE_SPDZ2_SLACK:
      cout << "Computing slack for active SPDZ2 proof" << endl;
      return EncCommit_<FFT_Data>::active_slack(phim);
    default:
      if (slack < 0)
        throw runtime_error("slack type unknown");
      return bigint(1) << slack;
  }
}

void Proof::get_challenge(vector<int>& e, const octetStream& ciphertexts) const
{
  unsigned int i;
  bigint hashout = ciphertexts.check_sum();

  for (i=0; i<sec; i++)
    { e[i]=(hashout.get_ui()>>(i))&1; }
}

class AbsoluteBoundChecker
{
  bigint bound, neg_bound;

public:
  AbsoluteBoundChecker(bigint bound) : bound(bound), neg_bound(-bound) {}
  bool outside(const bigint& value, double& dist)
  {
    (void)dist;
#ifdef PRINT_MIN_DIST
    dist = max(dist, abs(value.get_d()) / bound.get_d());
#endif
    return value > bound || value < neg_bound;
  }
};

template <class T>
bool Proof::check_bounds(T& z, AddableMatrix<bigint>& t, int i) const
{
  unsigned int j,k;

  // Check Bound 1 and Bound 2
  AbsoluteBoundChecker plain_checker(plain_check * n_proofs);
  AbsoluteBoundChecker rand_checker(rand_check * n_proofs);
  for (j=0; j<phim; j++)
    {
      const bigint& te = z[j];
      if (plain_checker.outside(te, dist))
        {
          cout << "Fail on Check 1 " << i << " " << j << endl;
          cout << te << "  " << plain_check << endl;
          cout << tau << " " << sec << " " << n_proofs << endl;
          return false;
        }
    }
  for (k=0; k<3; k++)
    {
      const vector<bigint>& coeffs = t[k];
      for (j=0; j<coeffs.size(); j++)
        {
          const bigint& te = coeffs.at(j);
          if (rand_checker.outside(te, dist))
            {
              cout << "Fail on Check 2 " << k << " : " << i << " " << j << endl;
              cout << te << "  " << rand_check << endl;
              cout << rho << " " << sec << " " << n_proofs << endl;
              return false;
            }
        }
    }
  return true;
}

Proof::Preimages::Preimages(int size, const FHE_PK& pk, const bigint& p, int n_players) :
    r(size, pk.get_params())
{
  m.resize(size, pk.get_params().phi_m());
  // extra limb for addition
  bigint limit = p << (64 + n_players);
  m.allocate_slots(limit);
  r.allocate_slots(limit);
  m_tmp = m[0][0];
  r_tmp = r[0][0];
}

void Proof::Preimages::add(octetStream& os)
{
  check_sizes();
  unsigned int size;
  os.get(size);
  if (size != m.size())
    throw length_error("unexpected size received");
  for (size_t i = 0; i < m.size(); i++)
    {
      m[i].add(os, m_tmp);
      r[i].add(os, r_tmp);
    }
}

void Proof::Preimages::pack(octetStream& os)
{
  check_sizes();
  os.store((unsigned int)m.size());
  for (size_t i = 0; i < m.size(); i++)
    {
      m[i].pack(os);
      r[i].pack(os);
    }
}

void Proof::Preimages::unpack(octetStream& os)
{
  unsigned int size;
  os.get(size);
  m.resize(size);
  if (size != r.size())
    throw runtime_error("unexpected size of preimage randomness");
  for (size_t i = 0; i < m.size(); i++)
    {
      m[i].unpack(os);
      r[i].unpack(os);
    }
}

void Proof::Preimages::check_sizes()
{
  if (m.size() != r.size())
    throw runtime_error("preimage sizes don't match");
}

template bool Proof::check_bounds(Plaintext<gfp,FFT_Data,bigint>& z, AddableMatrix<bigint>& t, int i) const;
template bool Proof::check_bounds(AddableVector<bigint>& z, AddableMatrix<bigint>& t, int i) const;

template bool Proof::check_bounds(Plaintext_<P2Data>& z, AddableMatrix<bigint>& t, int i) const;
