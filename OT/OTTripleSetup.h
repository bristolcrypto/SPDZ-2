// (C) 2018 University of Bristol. See License.txt

#ifndef OT_TRIPLESETUP_H_
#define OT_TRIPLESETUP_H_

#include "Networking/Player.h"
#include "OT/BaseOT.h"
#include "OT/OTMachine.h"
#include "Tools/random.h"
#include "Tools/time-func.h"
#include "Math/gfp.h"

/*
 * Class for creating and storing base OTs between every pair of parties.
 */
class OTTripleSetup
{
    vector<int> base_receiver_inputs;
    vector< vector< vector<BitVector> > > baseSenderInputs;
    vector< vector<BitVector> > baseReceiverOutputs;

    PRNG G;
    int nparties;
    int my_num;
    int nbase;

public:
    map<string,Timer> timers;
    vector<BaseOT*> baseOTs;
    vector<TwoPartyPlayer*> players;

    int get_nparties() { return nparties; }
    int get_nbase() { return nbase; }
    int get_my_num() { return my_num; }
    int get_base_receiver_input(int i) { return base_receiver_inputs[i]; }

    OTTripleSetup(Names& N, bool real_OTs)
        : nparties(N.num_players()), my_num(N.my_num()), nbase(128)
    {
        base_receiver_inputs.resize(nbase);
        players.resize(nparties - 1);
        baseOTs.resize(nparties - 1);
        baseSenderInputs.resize(nparties - 1);
        baseReceiverOutputs.resize(nparties - 1);

        if (real_OTs)
            cout << "Doing real base OTs\n";
        else
            cout << "Doing fake base OTs\n";

        for (int i = 0; i < nparties - 1; i++)
        {
            int other_player, id;
            // i for indexing, other_player is actual number
            if (i >= my_num)
                other_player = i + 1;
            else
                other_player = i;
            // unique id per pair of parties (to assign port no.)
            if (my_num < other_player)
                id = my_num*nparties + other_player;
            else
                id = other_player*nparties + my_num;

            players[i] = new TwoPartyPlayer(N, other_player, id);

            // sets up a pair of base OTs, playing both roles
            if (real_OTs)
            {
                baseOTs[i] = new BaseOT(nbase, 128, players[i]);
            }
            else
            {
                baseOTs[i] = new FakeOT(nbase, 128, players[i]);
            }
        }
    }
    ~OTTripleSetup();

    // run the Base OTs
    void setup();
    // close down the sockets
    void close_connections();

    //template <class T>
    //T get_mac_key();
};


#endif
