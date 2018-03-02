// (C) 2018 University of Bristol. See License.txt

#include "OT/BaseOT.h"
#include "Tools/random.h"
#include "Tools/benchmarking.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <pthread.h>

extern "C" {
#include "SimpleOT/ot_sender.h"
#include "SimpleOT/ot_receiver.h"
}

using namespace std;

const char* role_to_str(OT_ROLE role)
{
    if (role == RECEIVER)
        return "RECEIVER";
    if (role == SENDER)
        return "SENDER";
    return "BOTH";
}

OT_ROLE INV_ROLE(OT_ROLE role)
{
    if (role == RECEIVER)
        return SENDER;
    if (role == SENDER)
        return RECEIVER;
    else
        return BOTH;
}

void send_if_ot_sender(TwoPartyPlayer* P, vector<octetStream>& os, OT_ROLE role)
{
    if (role == SENDER)
    {
        P->send(os[0]);
    }
    else if (role == RECEIVER)
    {
        P->receive(os[1]);
    }
    else
    {
        // both sender + receiver
        P->send_receive_player(os);
    }
}

void send_if_ot_receiver(TwoPartyPlayer* P, vector<octetStream>& os, OT_ROLE role)
{
    if (role == RECEIVER)
    {
        P->send(os[0]);
    }
    else if (role == SENDER)
    {
        P->receive(os[1]);
    }
    else
    {
        // both
        P->send_receive_player(os);
    }
}


void BaseOT::exec_base(bool new_receiver_inputs)
{
    int i, j, k;
    size_t len;
    PRNG G;
    G.ReSeed();
    vector<octetStream> os(2);
    SIMPLEOT_SENDER sender;
    SIMPLEOT_RECEIVER receiver;

    unsigned char S_pack[ PACKBYTES ];
    unsigned char Rs_pack[ 2 ][ 4 * PACKBYTES ];
    unsigned char sender_keys[ 2 ][ 4 ][ HASHBYTES ];
    unsigned char receiver_keys[ 4 ][ HASHBYTES ];
    unsigned char cs[ 4 ];

    if (ot_role & SENDER)
    {
        sender_genS(&sender, S_pack);
        os[0].store_bytes(S_pack, sizeof(S_pack));
    }
    send_if_ot_sender(P, os, ot_role);

    if (ot_role & RECEIVER)
    {
        os[1].get_bytes((octet*) receiver.S_pack, len);
        if (len != HASHBYTES)
        {
            cerr << "Received invalid length in base OT\n";
            exit(1);
        }
        receiver_procS(&receiver);
        receiver_maketable(&receiver);
    }

    for (i = 0; i < nOT; i += 4)
    {
        if (ot_role & RECEIVER)
        {
            for (j = 0; j < 4; j++)
            {
                if (new_receiver_inputs)
                    receiver_inputs[i + j] = G.get_uchar()&1;
                cs[j] = receiver_inputs[i + j];
            }
            receiver_rsgen(&receiver, Rs_pack[0], cs);
            os[0].reset_write_head();
            os[0].store_bytes(Rs_pack[0], sizeof(Rs_pack[0]));
            receiver_keygen(&receiver, receiver_keys);
        }
        send_if_ot_receiver(P, os, ot_role);
        
        if (ot_role & SENDER)
        {
            os[1].get_bytes((octet*) Rs_pack[1], len);
            if (len != sizeof(Rs_pack[1]))
            {
                cerr << "Received invalid length in base OT\n";
                exit(1);
            }
            sender_keygen(&sender, Rs_pack[1], sender_keys);

            // Copy 128 bits of keys to sender_inputs
            for (j = 0; j < 4; j++)
            {
                for (k = 0; k < AES_BLK_SIZE; k++)
                {
                    sender_inputs[i + j][0].set_byte(k, sender_keys[0][j][k]);
                    sender_inputs[i + j][1].set_byte(k, sender_keys[1][j][k]);
                }
            }
        }
        
        if (ot_role & RECEIVER)
        {
            // Copy keys to receiver_outputs
            for (j = 0; j < 4; j++)
            {
                for (k = 0; k < AES_BLK_SIZE; k++)
                {
                    receiver_outputs[i + j].set_byte(k, receiver_keys[j][k]);
                }
            }
        }
        #ifdef BASE_OT_DEBUG
        for (j = 0; j < 4; j++)
        {
            if (ot_role & SENDER)
            {
                printf("%4d-th sender keys:", i+j);
                for (k = 0; k < HASHBYTES; k++) printf("%.2X", sender_keys[0][j][k]);
                printf(" ");
                for (k = 0; k < HASHBYTES; k++) printf("%.2X", sender_keys[1][j][k]);
                printf("\n");
            }
            if (ot_role & RECEIVER)
            {
                printf("%4d-th receiver key:", i+j);
                for (k = 0; k < HASHBYTES; k++) printf("%.2X", receiver_keys[j][k]);
                printf("\n");
            }
        }

        printf("\n");
        #endif
    }
    set_seeds();
}

void BaseOT::set_seeds()
{
    for (int i = 0; i < nOT; i++)
    {
        // Set PRG seeds
        if (ot_role & SENDER)
        {
            G_sender[i][0].SetSeed(sender_inputs[i][0].get_ptr());
            G_sender[i][1].SetSeed(sender_inputs[i][1].get_ptr());
        }
        if (ot_role & RECEIVER)
        {
            G_receiver[i].SetSeed(receiver_outputs[i].get_ptr());
        }
    }
    extend_length();
}

void BaseOT::extend_length()
{
    for (int i = 0; i < nOT; i++)
    {
        if (ot_role & SENDER)
        {
            sender_inputs[i][0].randomize(G_sender[i][0]);
            sender_inputs[i][1].randomize(G_sender[i][1]);
        }
        if (ot_role & RECEIVER)
        {
            receiver_outputs[i].randomize(G_receiver[i]);
        }
    }
}


void BaseOT::check()
{
    vector<octetStream> os(2);
    BitVector tmp_vector(8 * AES_BLK_SIZE);


    for (int i = 0; i < nOT; i++)
    {
        if (ot_role == SENDER)
        {
            // send both inputs over
            sender_inputs[i][0].pack(os[0]);
            sender_inputs[i][1].pack(os[0]);
            P->send(os[0]);
        }
        else if (ot_role == RECEIVER)
        {
            P->receive(os[1]);
        }
        else
        {
            // both sender + receiver
            sender_inputs[i][0].pack(os[0]);
            sender_inputs[i][1].pack(os[0]);
            P->send_receive_player(os);
        }
        if (ot_role & RECEIVER)
        {
            tmp_vector.unpack(os[1]);
        
            if (receiver_inputs[i] == 1)
            {
                tmp_vector.unpack(os[1]);
            }
            if (!tmp_vector.equals(receiver_outputs[i]))
            {
                cerr << "Incorrect OT\n";
                exit(1);
            }
        }
        os[0].reset_write_head();
        os[1].reset_write_head();
    }
}


void FakeOT::exec_base(bool new_receiver_inputs)
{
    insecure("base OTs");
    PRNG G;
    G.ReSeed();
    vector<octetStream> os(2);
    vector<BitVector> bv(2);

    if ((ot_role & RECEIVER) && new_receiver_inputs)
    {
        for (int i = 0; i < nOT; i++)
            // Generate my receiver inputs
            receiver_inputs[i] = G.get_uchar()&1;
    }

    if (ot_role & SENDER)
        for (int i = 0; i < nOT; i++)
            for (int j = 0; j < 2; j++)
            {
                sender_inputs[i][j].randomize(G);
                sender_inputs[i][j].pack(os[0]);
            }

    send_if_ot_sender(P, os, ot_role);

    if (ot_role & RECEIVER)
        for (int i = 0; i < nOT; i++)
        {
            for (int j = 0; j < 2; j++)
                bv[j].unpack(os[1]);
            receiver_outputs[i] = bv[receiver_inputs[i]];
        }

    set_seeds();
}
