// (C) 2018 University of Bristol. See License.txt

#ifndef _DistDecrypt
#define _DistDecrypt

/*
 * This is the Distributed Decryption Protocol
 *
 */

#include "FHE/Ciphertext.h"
#include "Networking/Player.h"
#include "FHEOffline/Reshare.h"

template<class FD>
class DistDecrypt
{
protected:
  const Player& P;
  const FHE_SK& share;
  const FHE_PK& pk;
  AddableVector<bigint> vv, vv1;

public:
  Plaintext_<FD> mf, f;

  DistDecrypt(const Player& P, const FHE_SK& share, const FHE_PK& pk,
      const FD& fd);
  virtual ~DistDecrypt() {}

  Plaintext_<FD>& run(const Ciphertext& ctx, bool NewCiphertext = false);
  virtual void intermediate_step() {}

  virtual void reshare(Plaintext<typename FD::T, FD, typename FD::S>& m,
      const Ciphertext& cm,
      EncCommitBase<typename FD::T, FD, typename FD::S>& EC)
  { Ciphertext dummy(pk.get_params()); Reshare(m, dummy, cm, false, P, EC, pk, *this); }

  size_t report_size(ReportType type)
  { return vv.report_size(type) + vv1.report_size(type) + mf.report_size(type) + f.report_size(type); }
};

#endif

