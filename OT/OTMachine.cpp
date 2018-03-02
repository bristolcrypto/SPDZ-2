// (C) 2018 University of Bristol. See License.txt

#include "Networking/Player.h"
#include "OT/OTExtension.h"
#include "OT/OTExtensionWithMatrix.h"
#include "Exceptions/Exceptions.h"
#include "Tools/time-func.h"

#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <iostream>

#include <sys/time.h>

#include "OutputCheck.h"
#include "OTMachine.h"

//#define BASE_OT_DEBUG

class OT_thread_info
{
    public:
 
    int thread_num;
    bool stop;
    int other_player_num;
    OTExtension* ot_ext;
    int nOTs, nbase;
    BitVector receiverInput;
};

void* run_otext_thread(void* ptr)
{
    OT_thread_info *tinfo = (OT_thread_info*) ptr;

    //int num = tinfo->thread_num;
    //int other_player_num = tinfo->other_player_num;
    printf("\tI am in thread %d\n", tinfo->thread_num);
    tinfo->ot_ext->transfer(tinfo->nOTs, tinfo->receiverInput);
    return NULL;
}

OTMachine::OTMachine(int argc, const char** argv)
{
    opt.add(
        "", // Default.
        1, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "This player's number, 0/1 (required).", // Help description.
        "-p", // Flag token.
        "--player" // Flag token.
    );

    opt.add(
        "5000", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Base port number (default: 5000).", // Help description.
        "-pn", // Flag token.
        "--portnum" // Flag token.
    );

    opt.add(
        "localhost", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Host name(s) that player 0 is running on (default: localhost). Split with commas.", // Help description.
        "-h", // Flag token.
        "--hostname" // Flag token.
    );

    opt.add(
        "1024",
        0,
        1,
        0,
        "Number of extended OTs to run (default: 1024).",
        "-n",
        "--nOTs"
    );

    opt.add(
        "128", // Default.
        0, // Required?
        1, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Number of base OTs to run (default: 128).", // Help description.
        "-b", // Flag token.
        "--nbase" // Flag token.
    );

    opt.add(
        "s",
        0,
        1,
        0,
        "Mode for OT. a (asymmetric) or s (symmetric, i.e. play both sender/receiver) (default: s).",
        "-m",
        "--mode"
    );
    opt.add(
        "1",
        0,
        1,
        0,
        "Number of threads (default: 1).",
        "-x",
        "--nthreads"
    );

    opt.add(
        "1",
        0,
        1,
        0,
        "Number of loops (default: 1).",
        "-l",
        "--nloops"
    );

    opt.add(
        "1",
        0,
        1,
        0,
        "Number of subloops (default: 1).",
        "-s",
        "--nsubloops"
    );

    opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Run in passive security mode.", // Help description.
        "-pas", // Flag token.
        "--passive" // Flag token.
    );

    opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Write results to files.", // Help description.
        "-o", // Flag token.
        "--output" // Flag token.
    );

    opt.add(
        "", // Default.
        0, // Required?
        0, // Number of args expected.
        0, // Delimiter if expecting multiple args.
        "Real base OT.", // Help description.
        "-r", // Flag token.
        "--real" // Flag token.
    );

    opt.parse(argc, argv);

    string hostname, ot_mode, usage;
    passive = false;
    opt.get("-p")->getInt(my_num);
    opt.get("-pn")->getInt(portnum_base);
    opt.get("-h")->getString(hostname);
    opt.get("-n")->getLong(nOTs);
    opt.get("-m")->getString(ot_mode);
    opt.get("--nthreads")->getInt(nthreads);
    opt.get("--nloops")->getInt(nloops);
    opt.get("--nsubloops")->getInt(nsubloops);
    opt.get("--nbase")->getInt(nbase);
    if (opt.isSet("-pas"))
        passive = true;

    if (!opt.isSet("-p"))
    {
        opt.getUsage(usage);
        cout << usage;
        exit(0);
    }

    cout << "Player 0 host name = " << hostname << endl;
    cout << "Creating " << nOTs << " extended OTs in " << nthreads << " threads\n";
    cout << "Running in mode " << ot_mode << endl;

    if (passive)
        cout << "Running with PASSIVE security only\n";

    if (nbase < 128)
        cout << "WARNING: only using " << nbase << " seed OTs, using these for OT extensions is insecure.\n";

    if (ot_mode.compare("s") == 0)
        ot_role = BOTH;
    else if (ot_mode.compare("a") == 0)
    {
        if (my_num == 0)
            ot_role = SENDER;
        else
            ot_role = RECEIVER;
    }
    else
    {
        cerr << "Invalid OT mode argument: " << ot_mode << endl;
        exit(1);
    }

    // Several names for multiplexing
    unsigned int pos = 0;
    while (pos < hostname.length())
    {
        string::size_type new_pos = hostname.find(',', pos);
        if (new_pos == string::npos)
            new_pos = hostname.length();
        int len = new_pos - pos;
        string name = hostname.substr(pos, len);
        pos = new_pos + 1;

        vector<string> names(2);
        names[my_num] = "localhost";
        names[1-my_num] = name;
        N.push_back(new Names(my_num, portnum_base + 1000 * N.size(), names));
    }

    P = new TwoPartyPlayer(*N[0], 1 - my_num, 500);

    timeval baseOTstart, baseOTend;
    gettimeofday(&baseOTstart, NULL);
    // swap role for base OTs
    if (opt.isSet("-r"))
        bot_ = new BaseOT(nbase, 128, P, INV_ROLE(ot_role));
    else
        bot_ = new FakeOT(nbase, 128, P, INV_ROLE(ot_role));
    cout << "real mode " << opt.isSet("-r") << endl;
    BaseOT& bot = *bot_;
    bot.exec_base();
    gettimeofday(&baseOTend, NULL);
    double basetime = timeval_diff(&baseOTstart, &baseOTend);
    cout << "\t\tBaseTime (" << role_to_str(ot_role) << "): " << basetime/1000000 << endl << flush;

    // Receiver send something to force synchronization
    // (since Sender finishes baseOTs before Receiver)
    int a = 3;
    vector<octetStream> os(2);
    os[0].store(a);
    P->send_receive_player(os);
    os[1].get(a);
    cout << a << endl;

#ifdef BASE_OT_DEBUG
    // check base OTs
    bot.check();
    // check after extending with PRG a few times
    for (int i = 0; i < 8; i++)
    {
        bot.extend_length();
        bot.check();
    }
    cout << "Verifying base OTs (debugging)\n";
#endif

    // convert baseOT selection bits to BitVector
    // (not already BitVector due to legacy PVW code)
    baseReceiverInput.resize(nbase);
    for (int i = 0; i < nbase; i++)
    {
        baseReceiverInput.set_bit(i, bot.receiver_inputs[i]);
    }
}

OTMachine::~OTMachine()
{
    for (auto names : N)
        delete names;
    delete bot_;
    delete P;
}


void OTMachine::run()
{
    // divide nOTs between threads and loops
    nOTs = DIV_CEIL(nOTs, nthreads * nloops);
    // round up to multiple of base OTs and subloops
    // discount for discarded OTs
    nOTs = DIV_CEIL(nOTs + 2 * 128, nbase * nsubloops) * nbase * nsubloops - 2 * 128;
    cout << "Running " << nOTs << " OT extensions per thread and loop\n" << flush;

    // PRG for generating inputs etc
    PRNG G;
    G.ReSeed();
    BitVector receiverInput(nOTs);
    receiverInput.randomize(G);
    BaseOT& bot = *bot_;

    cout << "Initialize OT Extension\n";
    vector<OT_thread_info> tinfos(nthreads);
    vector<pthread_t> threads(nthreads);
    timeval OTextstart, OTextend;
    gettimeofday(&OTextstart, NULL);

    // copy base inputs/outputs for each thread
    vector<BitVector> base_receiver_input_copy(nthreads);
    vector<vector< vector<BitVector> > > base_sender_inputs_copy(nthreads, vector<vector<BitVector> >(nbase, vector<BitVector>(2)));
    vector< vector<BitVector> > base_receiver_outputs_copy(nthreads, vector<BitVector>(nbase));
    vector<TwoPartyPlayer*> players(nthreads);

    for (int i = 0; i < nthreads; i++)
    {
        tinfos[i].receiverInput.assign(receiverInput);

        base_receiver_input_copy[i].assign(baseReceiverInput);
        for (int j = 0; j < nbase; j++)
        {
            base_sender_inputs_copy[i][j][0].assign(bot.sender_inputs[j][0]);
            base_sender_inputs_copy[i][j][1].assign(bot.sender_inputs[j][1]);
            base_receiver_outputs_copy[i][j].assign(bot.receiver_outputs[j]);
        }
        // now setup resources for each thread
        // round robin with the names
        players[i] = new TwoPartyPlayer(*N[i%N.size()], 1 - my_num, (i+1) * 1000);
        tinfos[i].thread_num = i+1;
        tinfos[i].other_player_num = 1 - my_num;
        tinfos[i].nOTs = nOTs;
        tinfos[i].ot_ext = new OTExtensionWithMatrix(nbase, bot.length(),
                nloops, nsubloops,
                players[i],
                base_receiver_input_copy[i],
                base_sender_inputs_copy[i],
                base_receiver_outputs_copy[i],
                ot_role,
                passive);

        // create the thread
        pthread_create(&threads[i], NULL, run_otext_thread, &tinfos[i]);
        
        // extend base OTs with PRG for the next thread
        bot.extend_length();
    }
    // wait for threads to finish
    for (int i = 0; i < nthreads; i++)
    {
        pthread_join(threads[i],NULL);
        cout << "thread " << i+1 << " finished\n" << flush;
    }

    map<string,long long>& times = tinfos[0].ot_ext->times;
    for (map<string,long long>::iterator it = times.begin(); it != times.end(); it++)
    {
        long long sum = 0;
        for (int i = 0; i < nthreads; i++)
            sum += tinfos[i].ot_ext->times[it->first];

        cout << it->first << " on average took time "
                << double(sum) / nthreads / 1e6 << endl;
    }

    gettimeofday(&OTextend, NULL);
    double totaltime = timeval_diff(&OTextstart, &OTextend);
    cout << "Time for OTExt threads (" << role_to_str(ot_role) << "): " << totaltime/1000000 << endl << flush;

    if (opt.isSet("-o"))
    {
        BitVector receiver_output, sender_output;
        char filename[1024];
        sprintf(filename, RECEIVER_INPUT, my_num);
        ofstream outf(filename);
        receiverInput.output(outf, false);
        outf.close();
        sprintf(filename, RECEIVER_OUTPUT, my_num);
        outf.open(filename);
        for (unsigned int i = 0; i < nOTs; i++)
        {
            receiver_output.assign_bytes((char*) tinfos[0].ot_ext->get_receiver_output(i), sizeof(__m128i));
            receiver_output.output(outf, false);
        }
        outf.close();

        for (int i = 0; i < 2; i++)
        {
            sprintf(filename, SENDER_OUTPUT, my_num, i);
            outf.open(filename);
            for (int j = 0; j < nOTs; j++)
            {
                sender_output.assign_bytes((char*) tinfos[0].ot_ext->get_sender_output(i, j), sizeof(__m128i));
                sender_output.output(outf, false);
            }
            outf.close();
        }
    }

    for (int i = 0; i < nthreads; i++)
    {
        delete players[i];
        delete tinfos[i].ot_ext;
    }
}
