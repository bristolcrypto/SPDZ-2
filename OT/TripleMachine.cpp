// (C) 2018 University of Bristol. See License.txt

/*
 * TripleMachine.cpp
 *
 */

#include <OT/TripleMachine.h>
#include "OT/NPartyTripleGenerator.h"
#include "OT/OTMachine.h"
#include "OT/OTTripleSetup.h"
#include "Math/gf2n.h"
#include "Math/Setup.h"
#include "Tools/ezOptionParser.h"
#include "Math/Setup.h"

#include <iostream>
#include <fstream>
using namespace std;

template <class T>
void* run_ngenerator_thread(void* ptr)
{
    ((NPartyTripleGenerator*)ptr)->generate<T>();
    return 0;
}

TripleMachine::TripleMachine(int argc, const char** argv) :
        bonding(0), generateBits(0)
{
    opt.add(
        "1", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Number of loops (default: 1).", // Help description.
        "-l", // Flag token.
        "--nloops" // Flag token.
    );
    opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Generate MACs (implies -a).", // Help description.
        "-m", // Flag token.
        "--macs" // Flag token.
    );
    opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Amplify triples.", // Help description.
        "-a", // Flag token.
        "--amplify" // Flag token.
    );
    opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Check triples (implies -m).", // Help description.
        "-c", // Flag token.
        "--check" // Flag token.
    );
    opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "GF(p) triples", // Help description.
        "-P", // Flag token.
        "--prime-field" // Flag token.
    );
    opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Channel bonding", // Help description.
        "-b", // Flag token.
        "--bonding" // Flag token.
    );
    opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Generate bits", // Help description.
        "-B", // Flag token.
        "--bits" // Flag token.
    );

    parse_options(argc, argv);

    opt.get("-l")->getInt(nloops);
    generateBits = opt.get("-B")->isSet;
    check = opt.get("-c")->isSet || generateBits;
    generateMACs = opt.get("-m")->isSet || check;
    amplify = opt.get("-a")->isSet || generateMACs;
    primeField = opt.get("-P")->isSet;
    bonding = opt.get("-b")->isSet;

    bigint p;
    if (output)
    {
        prep_data_dir = get_prep_dir(nplayers, 128, 128);
        ofstream outf;
        generate_online_setup(outf, prep_data_dir, p, 128, 128);
    }
    else
    {
        int idx, m;
        SPDZ_Data_Setup_Primes(p, 128, idx, m);
    }

    // doesn't work with Montgomery multiplication
    gfp::init_field(p, false);
    gf2n::init_field(128);
    
    PRNG G;
    G.ReSeed();
    mac_key2.randomize(G);
    mac_keyp.randomize(G);
}

void TripleMachine::run()
{
    cout << "my_num: " << my_num << endl;
    Names N[2];
    N[0].init(my_num, 10000, "HOSTS", nplayers);
    int nConnections = 1;
    if (bonding)
    {
        N[1].init(my_num, 11000, "HOSTS2", nplayers);
        nConnections = 2;
    }
    // do the base OTs
    OTTripleSetup setup(N[0], true);
    setup.setup();
    setup.close_connections();

    vector<NPartyTripleGenerator*> generators(nthreads);
    vector<pthread_t> threads(nthreads);

    for (int i = 0; i < nthreads; i++)
    {
        generators[i] = new NPartyTripleGenerator(setup, N[i%nConnections], i, nTriplesPerThread, nloops, *this);
    }
    ntriples = generators[0]->nTriples * nthreads;
    cout <<"Setup generators\n";
    for (int i = 0; i < nthreads; i++)
    {
        // lock before starting thread to avoid race condition
        generators[i]->lock();
        if (primeField)
            pthread_create(&threads[i], 0, run_ngenerator_thread<gfp>, generators[i]);
        else
            pthread_create(&threads[i], 0, run_ngenerator_thread<gf2n>, generators[i]);
    }

    // wait for initialization, then start clock and computation
    for (int i = 0; i < nthreads; i++)
        generators[i]->wait();
    cout << "Starting computation" << endl;
    gettimeofday(&start, 0);
    for (int i = 0; i < nthreads; i++)
    {
        generators[i]->signal();
        generators[i]->unlock();
    }

    // wait for threads to finish
    for (int i = 0; i < nthreads; i++)
    {
        pthread_join(threads[i],NULL);
        cout << "thread " << i+1 << " finished\n" << flush;
    }

    map<string,Timer>& timers = generators[0]->timers;
    for (map<string,Timer>::iterator it = timers.begin(); it != timers.end(); it++)
    {
        double sum = 0;
        for (size_t i = 0; i < generators.size(); i++)
            sum += generators[i]->timers[it->first].elapsed();
        cout << it->first << " on average took time "
                << sum / generators.size() << endl;
    }

    gettimeofday(&stop, 0);
    double time = timeval_diff_in_seconds(&start, &stop);
    cout << "Time: " << time << endl;
    cout << "Throughput: " << ntriples / time << endl;

    for (size_t i = 0; i < generators.size(); i++)
        delete generators[i];

    if (output)
        output_mac_keys();
}

void TripleMachine::output_mac_keys()
{
    stringstream ss;
    ss << prep_data_dir << "Player-MAC-Keys-P" << my_num;
    cout << "Writing MAC key to " << ss.str() << endl;
    ofstream outputFile(ss.str().c_str());
    outputFile << nplayers << endl;
    outputFile << mac_keyp << " " << mac_key2 << endl;
}

template<> gf2n TripleMachine::get_mac_key()
{
    return mac_key2;
}

template<> gfp TripleMachine::get_mac_key()
{
    return mac_keyp;
}
