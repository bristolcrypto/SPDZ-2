// (C) 2018 University of Bristol. See License.txt

/*
 * PairwiseSetup.h
 *
 */

#ifndef FHEOFFLINE_PAIRWISESETUP_H_
#define FHEOFFLINE_PAIRWISESETUP_H_

#include "FHE/FHE_Params.h"
#include "FHE/Plaintext.h"
#include "Networking/Player.h"

template <class FD>
class PairwiseSetup
{
public:
    FHE_Params params;
    FD FieldD;
    typename FD::T alphai;
    Plaintext_<FD> alpha;
    string dirname;

    PairwiseSetup() : params(0), alpha(FieldD) {}
   
    void init(const Player& P, int sec, int plaintext_length, int& extra_slack);
};

#endif /* FHEOFFLINE_PAIRWISESETUP_H_ */
