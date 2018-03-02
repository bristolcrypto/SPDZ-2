// (C) 2018 University of Bristol. See License.txt

#ifndef OT_NPARTYTRIPLEGENERATOR_H_
#define OT_NPARTYTRIPLEGENERATOR_H_

#include "Networking/Player.h"
#include "OT/BaseOT.h"
#include "Tools/random.h"
#include "Tools/time-func.h"
#include "Math/gfp.h"
#include "Auth/MAC_Check.h"

#include "OT/OTTripleSetup.h"
#include "OT/TripleMachine.h"
#include "OT/OTMultiplier.h"

#include <map>
#include <vector>

#define N_AMPLIFY 3

template <class T, int N>
class ShareTriple;

class NPartyTripleGenerator
{
    //OTTripleSetup* setup;
    Player globalPlayer;

    int thread_num;
    int my_num;
    int nbase;

    struct timeval last_lap;

    pthread_mutex_t mutex;
    pthread_cond_t ready;

    template <class T>
    void generateTriples(vector< OTMultiplier<T>* >& ot_multipliers, ofstream& outputFile);
    template <class T>
    void generateBits(vector< OTMultiplier<T>* >& ot_multipliers, ofstream& outputFile);
    template <class T, int N>
    void generateBitsFromTriples(vector<ShareTriple<T, N> >& triples,
            MAC_Check<T>& MC, ofstream& outputFile);

    template <class T>
    void start_progress(vector< OTMultiplier<T>* >& ot_multipliers);
    void print_progress(int k);

public:
    // TwoPartyPlayer's for OTs, n-party Player for sacrificing
    vector<TwoPartyPlayer*> players;
    //vector<OTMachine*> machines;
    BitVector baseReceiverInput; // same for every set of OTs
    vector< vector< vector<BitVector> > > baseSenderInputs;
    vector< vector<BitVector> > baseReceiverOutputs;
    vector<BitVector> valueBits;

    int nTriples;
    int nTriplesPerLoop;
    int nloops;
    int field_size;
    int nAmplify;
    int nPreampTriplesPerLoop;
    int repeat[3];
    int nparties;

    TripleMachine& machine;

    map<string,Timer> timers;

    NPartyTripleGenerator(OTTripleSetup& setup, const Names& names, int thread_num, int nTriples, int nloops, TripleMachine& machine);
    ~NPartyTripleGenerator();
    template <class T>
    void generate();

    void lock();
    void unlock();
    void signal();
    void wait();
};

#endif
