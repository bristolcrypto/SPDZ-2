// (C) 2017 University of Bristol. See License.txt

/*
 * TripleMachine.h
 *
 */

#ifndef OT_TRIPLEMACHINE_H_
#define OT_TRIPLEMACHINE_H_

#include "Math/gf2n.h"
#include "Math/gfp.h"

class TripleMachine
{
    gf2n mac_key2;
    gfp mac_keyp;

public:
    int my_num, nplayers, nthreads, ntriples, nloops;
    int nTriplesPerThread;
    string prep_data_dir;
    bool generateMACs;
    bool amplify;
    bool check;
    bool primeField;
    bool bonding;
    bool generateBits;
    bool output;
    struct timeval start, stop;

    TripleMachine(int argc, const char** argv);
    void run();

    template <class T>
    T get_mac_key();
    void output_mac_keys();
};

#endif /* OT_TRIPLEMACHINE_H_ */
