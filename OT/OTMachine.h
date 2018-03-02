// (C) 2018 University of Bristol. See License.txt

/*
 * OTMachine.h
 *
 */

#ifndef OT_OTMACHINE_H_
#define OT_OTMACHINE_H_

#include "OT/OTExtension.h"
#include "Tools/ezOptionParser.h"

class OTMachine
{
    ez::ezOptionParser opt;
    OT_ROLE ot_role;

public:
    int my_num, portnum_base, nthreads, nloops, nsubloops, nbase;
    long nOTs;
    bool passive;
    TwoPartyPlayer* P;
    BitVector baseReceiverInput;
    BaseOT* bot_;
    vector<Names*> N;

    OTMachine(int argc, const char** argv);
    ~OTMachine();
    void run();
};

#endif /* OT_OTMACHINE_H_ */
