// (C) 2018 University of Bristol. See License.txt

/*
 * PairwiseMachine.h
 *
 */

#ifndef FHEOFFLINE_PAIRWISEMACHINE_H_
#define FHEOFFLINE_PAIRWISEMACHINE_H_

#include "FHEOffline/PairwiseGenerator.h"
#include "FHEOffline/SimpleMachine.h"
#include "FHEOffline/PairwiseSetup.h"

class PairwiseMachine : public MachineBase
{
public:
    PairwiseSetup<FFT_Data> setup_p;
    PairwiseSetup<P2Data> setup_2;
    Player P;

    vector<FHE_PK> other_pks;
    FHE_PK& pk;
    FHE_SK sk;
    vector<Ciphertext> enc_alphas;

    PairwiseMachine(int argc, const char** argv);

    template <class FD>
    void setup_keys();

    template <class FD>
    PairwiseSetup<FD>& setup();
};

#endif /* FHEOFFLINE_PAIRWISEMACHINE_H_ */
