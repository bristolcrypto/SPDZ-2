// (C) 2018 University of Bristol. See License.txt

#ifndef _OTEXTENSION
#define _OTEXTENSION

#include "OT/BaseOT.h"

#include "Exceptions/Exceptions.h"

#include "Networking/Player.h"

#include "Tools/time-func.h"

#include <stdlib.h>
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>

using namespace std;

//#define OTEXT_TIMER
//#define OTEXT_DEBUG


class OTExtension
{
public:
    BitVector baseReceiverInput;
    vector< vector<BitVector> > senderOutput;
    vector<BitVector> receiverOutput;
    map<string,long long> times;

    OTExtension(int nbaseOTs, int baseLength,
                int nloops, int nsubloops,
                TwoPartyPlayer* player,
                BitVector& baseReceiverInput,
                vector< vector<BitVector> >& baseSenderInput,
                vector<BitVector>& baseReceiverOutput,
                OT_ROLE role=BOTH,
                bool passive=false)
        : baseReceiverInput(baseReceiverInput), passive_only(passive), nbaseOTs(nbaseOTs),
          baseLength(baseLength), nloops(nloops), nsubloops(nsubloops), ot_role(role), player(player)
    {
        G_sender.resize(nbaseOTs, vector<PRNG>(2));
        G_receiver.resize(nbaseOTs);

        // set up PRGs for expanding the seed OTs
        for (int i = 0; i < nbaseOTs; i++)
        {
            assert(baseSenderInput[i][0].size_bytes() >= AES_BLK_SIZE);
            assert(baseSenderInput[i][1].size_bytes() >= AES_BLK_SIZE);
            assert(baseReceiverOutput[i].size_bytes() >= AES_BLK_SIZE);

            if (ot_role & RECEIVER)
            {
                G_sender[i][0].SetSeed(baseSenderInput[i][0].get_ptr());
                G_sender[i][1].SetSeed(baseSenderInput[i][1].get_ptr());
            }
            if (ot_role & SENDER)
            {
                G_receiver[i].SetSeed(baseReceiverOutput[i].get_ptr());
            }

#ifdef OTEXT_DEBUG
            // sanity check for base OTs
            vector<octetStream> os(2);
            BitVector t0(128);

            if (ot_role & RECEIVER)
            {
                // send both inputs to test
                baseSenderInput[i][0].pack(os[0]);
                baseSenderInput[i][1].pack(os[0]);
            }
            send_if_ot_receiver(player, os, ot_role);

            if (ot_role & SENDER)
            {
                // sender checks results
                t0.unpack(os[1]);
                if (baseReceiverInput.get_bit(i) == 1)
                    t0.unpack(os[1]);
                if (!t0.equals(baseReceiverOutput[i]))
                {
                    cerr << "Incorrect base OT\n";
                    exit(1);
                }
            }
            

            os[0].reset_write_head();
            os[1].reset_write_head();
#endif
        }
    }

    virtual ~OTExtension() {}

    virtual void transfer(int nOTs, const BitVector& receiverInput);
    virtual octet* get_receiver_output(int i);
    virtual octet* get_sender_output(int choice, int i);

protected:
    bool passive_only;
    int nbaseOTs, baseLength, nloops, nsubloops;
    OT_ROLE ot_role;
    TwoPartyPlayer* player;
    vector< vector<PRNG> > G_sender;
    vector<PRNG> G_receiver;

    void check_correlation(int nOTs,
        const BitVector& receiverInput);

    void check_iteration(__m128i delta, __m128i q, __m128i q2,
        __m128i t, __m128i t2, __m128i x);

    void hash_outputs(int nOTs, vector<BitVector>& receiverOutput);
};

#endif
