// (C) 2018 University of Bristol. See License.txt

/*
 * TripleMachine.h
 *
 */

#ifndef OT_TRIPLEMACHINE_H_
#define OT_TRIPLEMACHINE_H_

#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Tools/OfflineMachineBase.h"

class TripleMachine : public OfflineMachineBase
{
    gf2n mac_key2;
    gfp mac_keyp;

public:
    int nloops;
    string prep_data_dir;
    bool generateMACs;
    bool amplify;
    bool check;
    bool primeField;
    bool bonding;
    bool generateBits;
    struct timeval start, stop;

    TripleMachine(int argc, const char** argv);
    void run();

    template <class T>
    T get_mac_key();
    void output_mac_keys();
};

#endif /* OT_TRIPLEMACHINE_H_ */
