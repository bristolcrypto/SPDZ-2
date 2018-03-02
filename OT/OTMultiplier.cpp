// (C) 2018 University of Bristol. See License.txt

/*
 * OTMultiplier.cpp
 *
 */

#include "OT/OTMultiplier.h"
#include "OT/NPartyTripleGenerator.h"

#include <math.h>

template<class T>
OTMultiplier<T>::OTMultiplier(NPartyTripleGenerator& generator,
        int thread_num) :
        generator(generator), thread_num(thread_num),
        rot_ext(128, 128, 0, 1,
                generator.players[thread_num], generator.baseReceiverInput,
                generator.baseSenderInputs[thread_num],
                generator.baseReceiverOutputs[thread_num], BOTH, !generator.machine.check)
{
    c_output.resize(generator.nTriplesPerLoop);
    pthread_mutex_init(&mutex, 0);
    pthread_cond_init(&ready, 0);
    thread = 0;
}

template<class T>
OTMultiplier<T>::~OTMultiplier()
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&ready);
}

template<class T>
void OTMultiplier<T>::multiply()
{
    BitVector keyBits(generator.field_size);
    keyBits.set_int128(0, generator.machine.get_mac_key<T>().to_m128i());
    rot_ext.extend<T>(generator.field_size, keyBits);
    vector< vector<BitVector> > senderOutput(128);
    vector<BitVector> receiverOutput;
    for (int j = 0; j < 128; j++)
    {
        senderOutput[j].resize(2);
        for (int i = 0; i < 2; i++)
        {
            senderOutput[j][i].resize(128);
            senderOutput[j][i].set_int128(0, rot_ext.senderOutputMatrices[i].squares[0].rows[j]);
        }
    }
    rot_ext.receiverOutputMatrix.to(receiverOutput);
    OTExtensionWithMatrix auth_ot_ext(128, 128, 0, 1,
            generator.players[thread_num], keyBits, senderOutput,
            receiverOutput, BOTH, true);

    if (generator.machine.generateBits)
    	multiplyForBits(auth_ot_ext);
    else
    	multiplyForTriples(auth_ot_ext);
}

template<class T>
void OTMultiplier<T>::multiplyForTriples(OTExtensionWithMatrix& auth_ot_ext)
{
    auth_ot_ext.resize(generator.nPreampTriplesPerLoop * generator.field_size);

    // dummy input for OT correlator
    vector<BitVector> _;
    vector< vector<BitVector> > __;
    BitVector ___;

    OTExtensionWithMatrix otCorrelator(0, 0, 0, 0, generator.players[thread_num],
            ___, __, _, BOTH, true);
    otCorrelator.resize(128 * generator.nPreampTriplesPerLoop);

    rot_ext.resize(generator.field_size * generator.nPreampTriplesPerLoop + 2 * 128);
    
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&ready);
    pthread_cond_wait(&ready, &mutex);

    for (int i = 0; i < generator.nloops; i++)
    {
        BitVector aBits = generator.valueBits[0];
        //timers["Extension"].start();
        rot_ext.extend<T>(generator.field_size * generator.nPreampTriplesPerLoop, aBits);
        //timers["Extension"].stop();

        //timers["Correlation"].start();
        otCorrelator.baseReceiverInput = aBits;
        otCorrelator.setup_for_correlation(rot_ext.senderOutputMatrices, rot_ext.receiverOutputMatrix);
        otCorrelator.correlate<T>(0, generator.nPreampTriplesPerLoop, generator.valueBits[1], false, generator.nAmplify);
        //timers["Correlation"].stop();

        //timers["Triple computation"].start();

        otCorrelator.reduce_squares(generator.nPreampTriplesPerLoop, c_output);

        pthread_cond_signal(&ready);
        pthread_cond_wait(&ready, &mutex);

        if (generator.machine.generateMACs)
        {
            macs.resize(3);
            for (int j = 0; j < 3; j++)
            {
                int nValues = generator.nTriplesPerLoop;
                if (generator.machine.check && (j % 2 == 0))
                    nValues *= 2;
                auth_ot_ext.expand<T>(0, nValues);
                auth_ot_ext.correlate<T>(0, nValues, generator.valueBits[j], true);
                auth_ot_ext.reduce_squares(nValues, macs[j]);
            }

            pthread_cond_signal(&ready);
            pthread_cond_wait(&ready, &mutex);
        }
    }

    pthread_mutex_unlock(&mutex);
}

template<>
void OTMultiplier<gfp>::multiplyForBits(OTExtensionWithMatrix& auth_ot_ext)
{
	multiplyForTriples(auth_ot_ext);
}

template<>
void OTMultiplier<gf2n>::multiplyForBits(OTExtensionWithMatrix& auth_ot_ext)
{
    int nBits = generator.nTriplesPerLoop + generator.field_size;
    int nBlocks = ceil(1.0 * nBits / generator.field_size);
    auth_ot_ext.resize(nBlocks * generator.field_size);
    macs.resize(1);
    macs[0].resize(nBits);

    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&ready);
    pthread_cond_wait(&ready, &mutex);

    for (int i = 0; i < generator.nloops; i++)
    {
        auth_ot_ext.expand<gf2n>(0, nBlocks);
        auth_ot_ext.correlate<gf2n>(0, nBlocks, generator.valueBits[0], true);
        auth_ot_ext.transpose(0, nBlocks);

        for (int j = 0; j < nBits; j++)
        {
            int128 r = auth_ot_ext.receiverOutputMatrix.squares[j/128].rows[j%128];
            int128 s = auth_ot_ext.senderOutputMatrices[0].squares[j/128].rows[j%128];
            macs[0][j] = r ^ s;
        }

        pthread_cond_signal(&ready);
        pthread_cond_wait(&ready, &mutex);
    }

    pthread_mutex_unlock(&mutex);
}

template class OTMultiplier<gf2n>;
template class OTMultiplier<gfp>;
