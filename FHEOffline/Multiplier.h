// (C) 2018 University of Bristol. See License.txt

/*
 * Multiplier.h
 *
 */

#ifndef FHEOFFLINE_MULTIPLIER_H_
#define FHEOFFLINE_MULTIPLIER_H_

#include "FHEOffline/SimpleEncCommit.h"
#include "FHE/AddableVector.h"
#include "Tools/MemoryUsage.h"

template <class FD>
using PlaintextVector = AddableVector< Plaintext_<FD> >;

template <class FD>
class PairwiseGenerator;
class PairwiseMachine;

template <class FD>
class Multiplier
{
    PairwiseGenerator<FD>& generator;
    PairwiseMachine& machine;
    OffsetPlayer P;
    int num_players, my_num;
    const FHE_PK& other_pk;
    const Ciphertext& other_enc_alpha;
    map<string, Timer>& timers;

    // temporary
    Ciphertext C, mask;
    Plaintext_<FD> product_share;
    Random_Coins rc;

    size_t volatile_capacity;
    MemoryUsage memory_usage;

public:
    Multiplier(int offset, PairwiseGenerator<FD>& generator);
    void multiply_and_add(Plaintext_<FD>& res, const Ciphertext& C,
            const Plaintext_<FD>& b);
    void multiply_and_add(Plaintext_<FD>& res, const Ciphertext& C,
            const Rq_Element& b);
    void multiply_alpha_and_add(Plaintext_<FD>& res, const Rq_Element& b);
    int get_offset() { return P.get_offset(); }
    size_t report_size(ReportType type);
    void report_size(ReportType type, MemoryUsage& res);
    size_t report_volatile() { return volatile_capacity; }
};

#endif /* FHEOFFLINE_MULTIPLIER_H_ */
