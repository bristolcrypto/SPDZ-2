// (C) 2018 University of Bristol. See License.txt

/*
 * PairwiseGenerator.h
 *
 */

#ifndef FHEOFFLINE_PAIRWISEGENERATOR_H_
#define FHEOFFLINE_PAIRWISEGENERATOR_H_

#include <vector>
using namespace std;

#include "FHEOffline/Multiplier.h"
#include "FHEOffline/SimpleGenerator.h"

class PairwiseMachine;

template <class FD>
class PairwiseGenerator : public GeneratorBase
{
    friend MultiEncCommit<FD>;

    PlaintextVector<FD> a, b, c;
    AddableVector<Rq_Element> b_mod_q;
    vector<Multiplier<FD>*> multipliers;
    TripleProducer_<FD> producer;
    MultiEncCommit<FD> EC;

    // temporary data
    AddableVector<Ciphertext> C;
    octetStream ciphertexts, cleartexts;

    size_t volatile_memory;

public:
    PairwiseMachine& machine;
    Player global_player;

    PairwiseGenerator(int thread_num, PairwiseMachine& machine);
    ~PairwiseGenerator();

    void run();
    size_t report_size(ReportType type);
    void report_size(ReportType type, MemoryUsage& res);
    size_t report_sent();
};

#endif /* FHEOFFLINE_PAIRWISEGENERATOR_H_ */
