// (C) 2018 University of Bristol. See License.txt

#ifndef _Prover
#define _Prover

#include "Proof.h"
#include "Tools/MemoryUsage.h"

/* Class for the prover */

template<class FD, class U>
class Prover
{
  /* Provers state */
  Proof::Randomness s;
  AddableVector< Plaintext_<FD> > y;

#ifdef LESS_ALLOC_MORE_MEM
  AddableVector<bigint> z;
  AddableMatrix<bigint> t;
#endif

public:
  size_t volatile_memory;

  Prover(Proof& proof, const FD& FieldD);

  void Stage_1(const Proof& P, octetStream& ciphertexts, const AddableVector<Ciphertext>& c,
      const FHE_PK& pk, bool Diag,
      bool binary = false);

  bool Stage_2(Proof& P, octetStream& cleartexts,
               const vector<U>& x,
               const Proof::Randomness& r,
               const vector<int>& e);

  /* Only has a non-interactive version using the ROM 
      - If Diag is true then the plaintexts x are assumed to be
        diagonal elements, i.e. x=(x_1,x_1,...,x_1)
  */
  size_t NIZKPoK(Proof& P, octetStream& ciphertexts, octetStream& cleartexts,
	       const FHE_PK& pk,
               const AddableVector<Ciphertext>& c,
               const vector<U>& x,
               const Proof::Randomness& r,
               bool Diag,bool binary=false);

  size_t report_size(ReportType type);
  void report_size(ReportType type, MemoryUsage& res);
};

#endif
