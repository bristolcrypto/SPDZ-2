// (C) 2018 University of Bristol. See License.txt

/*
 * OTExtensionWithMatrix.cpp
 *
 */

#include "OTExtensionWithMatrix.h"
#include "Math/gfp.h"

void OTExtensionWithMatrix::seed(vector<BitMatrix>& baseSenderInput,
        BitMatrix& baseReceiverOutput)
{
    nbaseOTs = baseReceiverInput.size();
    //cout << "nbaseOTs " << nbaseOTs << endl;
    G_sender.resize(nbaseOTs, vector<PRNG>(2));
    G_receiver.resize(nbaseOTs);

    // set up PRGs for expanding the seed OTs
    for (int i = 0; i < nbaseOTs; i++)
    {
        if (ot_role & RECEIVER)
        {
            G_sender[i][0].SetSeed((octet*)&baseSenderInput[0].squares[i/128].rows[i%128]);
            G_sender[i][1].SetSeed((octet*)&baseSenderInput[1].squares[i/128].rows[i%128]);
        }
        if (ot_role & SENDER)
        {
            G_receiver[i].SetSeed((octet*)&baseReceiverOutput.squares[i/128].rows[i%128]);
        }
    }
}

void OTExtensionWithMatrix::transfer(int nOTs,
        const BitVector& receiverInput)
{
#ifdef OTEXT_TIMER
    timeval totalstartv, totalendv;
    gettimeofday(&totalstartv, NULL);
#endif
    cout << "\tDoing " << nOTs << " extended OTs as " << role_to_str(ot_role) << endl;

    // resize to account for extra k OTs that are discarded
    BitVector newReceiverInput(nOTs);
    for (unsigned int i = 0; i < receiverInput.size_bytes(); i++)
    {
        newReceiverInput.set_byte(i, receiverInput.get_byte(i));
    }

    for (int loop = 0; loop < nloops; loop++)
    {
        extend<gf2n>(nOTs, newReceiverInput);
#ifdef OTEXT_TIMER
        gettimeofday(&totalendv, NULL);
        double elapsed = timeval_diff(&totalstartv, &totalendv);
        cout << "\t\tTotal thread time: " << elapsed/1000000 << endl << flush;
#endif
    }

#ifdef OTEXT_TIMER
    gettimeofday(&totalendv, NULL);
    times["Total thread"] +=  timeval_diff(&totalstartv, &totalendv);
#endif
}

void OTExtensionWithMatrix::resize(int nOTs)
{
    t1.resize(nOTs);
    u.resize(nOTs);
    senderOutputMatrices.resize(2);
    for (int i = 0; i < 2; i++)
        senderOutputMatrices[i].resize(nOTs);
    receiverOutputMatrix.resize(nOTs);
}

// the template is used to denote the field of the hash output
template <class T>
void OTExtensionWithMatrix::extend(int nOTs_requested, BitVector& newReceiverInput)
{
//    if (nOTs % nbaseOTs != 0)
//        throw invalid_length(); //"nOTs must be a multiple of nbaseOTs\n");
    if (nOTs_requested == 0)
        return;
    // add k + s to account for discarding k OTs
    int nOTs = nOTs_requested + 2 * 128;

    int slice = nOTs / nsubloops / 128;
    nOTs = slice * nsubloops * 128;
    resize(nOTs);
    newReceiverInput.resize(nOTs);

    // randomize last 128 + 128 bits that will be discarded
    for (int i = 0; i < 4; i++)
        newReceiverInput.set_word(nOTs/64 - i - 1, G.get_word());

    // subloop for first part to interleave communication with computation
    for (int start = 0; start < nOTs / 128; start += slice)
    {
        expand<gf2n>(start, slice);
        correlate<gf2n>(start, slice, newReceiverInput, true);
        transpose(start, slice);
    }

#ifdef OTEXT_TIMER
    double elapsed;
#endif
    // correlation check
    if (!passive_only)
    {
#ifdef OTEXT_TIMER
        timeval startv, endv;
        gettimeofday(&startv, NULL);
#endif
        check_correlation(nOTs, newReceiverInput);
#ifdef OTEXT_TIMER
        gettimeofday(&endv, NULL);
        elapsed = timeval_diff(&startv, &endv);
        cout << "\t\tTotal correlation check time: " << elapsed/1000000 << endl << flush;
        times["Total correlation check"] += timeval_diff(&startv, &endv);
#endif
    }

    hash_outputs<T>(nOTs);

    receiverOutputMatrix.resize(nOTs_requested);
    senderOutputMatrices[0].resize(nOTs_requested);
    senderOutputMatrices[1].resize(nOTs_requested);
    newReceiverInput.resize(nOTs_requested);
}

template <class T>
void OTExtensionWithMatrix::expand(int start, int slice)
{
    BitMatrixSlice receiverOutputSlice(receiverOutputMatrix, start, slice);
    BitMatrixSlice senderOutputSlices[2] = {
            BitMatrixSlice(senderOutputMatrices[0], start, slice),
            BitMatrixSlice(senderOutputMatrices[1], start, slice)
    };
    BitMatrixSlice t1Slice(t1, start, slice);

    // expand with PRG
    if (ot_role & RECEIVER)
    {
        for (int i = 0; i < nbaseOTs; i++)
        {
            receiverOutputSlice.randomize<T>(i, G_sender[i][0]);
            t1Slice.randomize<T>(i, G_sender[i][1]);
        }
    }

    if (ot_role & SENDER)
    {
        for (int i = 0; i < nbaseOTs; i++)
            // randomize base receiver output
            senderOutputSlices[0].randomize<T>(i, G_receiver[i]);
    }
}

template <class T>
void OTExtensionWithMatrix::expand_transposed()
{
    for (int i = 0; i < nbaseOTs; i++)
    {
        if (ot_role & RECEIVER)
        {
            receiverOutputMatrix.squares[i/128].randomize<T>(i % 128, G_sender[i][0]);
            t1.squares[i/128].randomize<T>(i % 128, G_sender[i][1]);
        }
        if (ot_role & SENDER)
        {
            senderOutputMatrices[0].squares[i/128].randomize<T>(i % 128, G_receiver[i]);
        }
    }
}

void OTExtensionWithMatrix::setup_for_correlation(vector<BitMatrix>& baseSenderOutputs, BitMatrix& baseReceiverOutput)
{
    receiverOutputMatrix = baseSenderOutputs[0];
    t1 = baseSenderOutputs[1];
    u.resize(t1.size());
    senderOutputMatrices.resize(2);
    senderOutputMatrices[0] = baseReceiverOutput;
}

template <class T>
void OTExtensionWithMatrix::correlate(int start, int slice,
        BitVector& newReceiverInput, bool useConstantBase, int repeat)
{
    vector<octetStream> os(2);

    BitMatrixSlice receiverOutputSlice(receiverOutputMatrix, start, slice);
    BitMatrixSlice senderOutputSlices[2] = {
            BitMatrixSlice(senderOutputMatrices[0], start, slice),
            BitMatrixSlice(senderOutputMatrices[1], start, slice)
    };
    BitMatrixSlice t1Slice(t1, start, slice);
    BitMatrixSlice uSlice(u, start, slice);

    // create correlation
    if (ot_role & RECEIVER)
    {
        t1Slice.rsub<T>(receiverOutputSlice);
        t1Slice.add<T>(newReceiverInput, repeat);
        t1Slice.pack(os[0]);

//        t1 = receiverOutputMatrix;
//        t1 ^= newReceiverInput;
//        receiverOutputMatrix.print_side_by_side(t1);
    }
#ifdef OTEXT_TIMER
    timeval commst1, commst2;
    gettimeofday(&commst1, NULL);
#endif
    // send t0 + t1 + x
    send_if_ot_receiver(player, os, ot_role);

    // sender adjusts using base receiver bits
    if (ot_role & SENDER)
    {
        // u = t0 + t1 + x
        uSlice.unpack(os[1]);
        senderOutputSlices[0].conditional_add<T>(baseReceiverInput, u, !useConstantBase);
    }
#ifdef OTEXT_TIMER
    gettimeofday(&commst2, NULL);
    double commstime = timeval_diff(&commst1, &commst2);
    cout << "\t\tCommunication took time " << commstime/1000000 << endl << flush;
    times["Communication"] += timeval_diff(&commst1, &commst2);
#endif
}

void OTExtensionWithMatrix::transpose(int start, int slice)
{
    BitMatrixSlice receiverOutputSlice(receiverOutputMatrix, start, slice);
    BitMatrixSlice senderOutputSlices[2] = {
            BitMatrixSlice(senderOutputMatrices[0], start, slice),
            BitMatrixSlice(senderOutputMatrices[1], start, slice)
    };

    // transpose t0[i] onto receiverOutput and tmp (q[i]) onto senderOutput[i][0]

    //cout << "Starting matrix transpose\n" << flush << endl;
#ifdef OTEXT_TIMER
    timeval transt1, transt2;
    gettimeofday(&transt1, NULL);
#endif
    // transpose in 128-bit chunks
    if (ot_role & RECEIVER)
        receiverOutputSlice.transpose();
    if (ot_role & SENDER)
        senderOutputSlices[0].transpose();

#ifdef OTEXT_TIMER
    gettimeofday(&transt2, NULL);
    double transtime = timeval_diff(&transt1, &transt2);
    cout << "\t\tMatrix transpose took time " << transtime/1000000 << endl << flush;
    times["Matrix transpose"] += timeval_diff(&transt1, &transt2);
#endif
}

/*
 * Hash outputs to make into random OT
 */
template <class T>
void OTExtensionWithMatrix::hash_outputs(int nOTs)
{
    //cout << "Hashing... " << flush;
    octetStream os, h_os(HASH_SIZE);
    square128 tmp;
    MMO mmo;
#ifdef OTEXT_TIMER
    timeval startv, endv;
    gettimeofday(&startv, NULL);
#endif

    for (int i = 0; i < nOTs / 128; i++)
    {
        if (ot_role & SENDER)
        {
            tmp = senderOutputMatrices[0].squares[i];
            tmp ^= baseReceiverInput;
            senderOutputMatrices[0].squares[i].hash_row_wise<T>(mmo, senderOutputMatrices[0].squares[i]);
            senderOutputMatrices[1].squares[i].hash_row_wise<T>(mmo, tmp);
        }
        if (ot_role & RECEIVER)
        {
            receiverOutputMatrix.squares[i].hash_row_wise<T>(mmo, receiverOutputMatrix.squares[i]);
        }
    }
    //cout << "done.\n";
#ifdef OTEXT_TIMER
    gettimeofday(&endv, NULL);
    double elapsed = timeval_diff(&startv, &endv);
    cout << "\t\tOT ext hashing took time " << elapsed/1000000 << endl << flush;
    times["Hashing"] += timeval_diff(&startv, &endv);
#endif
}

template <class T>
void OTExtensionWithMatrix::reduce_squares(unsigned int nTriples, vector<T>& output)
{
    if (receiverOutputMatrix.squares.size() < nTriples)
        throw invalid_length();
    output.resize(nTriples);
    for (unsigned int j = 0; j < nTriples; j++)
    {
        T c1, c2;
        receiverOutputMatrix.squares[j].to(c1);
        senderOutputMatrices[0].squares[j].to(c2);
        output[j] = c1 - c2;
    }
}

octet* OTExtensionWithMatrix::get_receiver_output(int i)
{
    return (octet*)&receiverOutputMatrix.squares[i/128].rows[i%128];
}

octet* OTExtensionWithMatrix::get_sender_output(int choice, int i)
{
    return (octet*)&senderOutputMatrices[choice].squares[i/128].rows[i%128];
}

void OTExtensionWithMatrix::print(BitVector& newReceiverInput, int i)
{
    if (player->my_num() == 0)
    {
        print_receiver<gf2n>(newReceiverInput, receiverOutputMatrix, i);
        print_sender(senderOutputMatrices[0].squares[i], senderOutputMatrices[1].squares[i]);
    }
    else
    {
        print_sender(senderOutputMatrices[0].squares[i], senderOutputMatrices[1].squares[i]);
        print_receiver<gf2n>(newReceiverInput, receiverOutputMatrix, i);
    }
}

template <class T>
void OTExtensionWithMatrix::print_receiver(BitVector& newReceiverInput, BitMatrix& matrix, int k, int offset)
{
    if (ot_role & RECEIVER)
    {
        for (int i = 0; i < 16; i++)
        {
            if (newReceiverInput.get_bit((offset + k) * 128 + i))
            {
                for (int j = 0; j < 33; j++)
                    cout << " ";
                cout << T(matrix.squares[k].rows[i]);
            }
            else
                cout << int128(matrix.squares[k].rows[i]);
            cout << endl;
        }
        cout << endl;
    }
}

void OTExtensionWithMatrix::print_sender(square128& square0, square128& square1)
{
    if (ot_role & SENDER)
    {
        for (int i = 0; i < 16; i++)
        {
            cout << int128(square0.rows[i]) << " ";
            cout << int128(square1.rows[i]) << " ";
            cout << endl;
        }
        cout << endl;
    }
}

template <class T>
void OTExtensionWithMatrix::print_post_correlate(BitVector& newReceiverInput, int j, int offset, int sender)
{
   cout << "post correlate, sender" << sender << endl;
   if (player->my_num() == sender)
   {
       T delta = newReceiverInput.get_int128(offset + j);
       for (int i = 0; i < 16; i++)
       {
           cout << (int128(receiverOutputMatrix.squares[j].rows[i]));
           cout << " ";
           cout << (T(receiverOutputMatrix.squares[j].rows[i]) - delta);
           cout << endl;
       }
       cout << endl;
   }
   else
   {
       print_receiver<T>(baseReceiverInput, senderOutputMatrices[0], j);
   }
}

void OTExtensionWithMatrix::print_pre_correlate(int i)
{
    cout << "pre correlate" << endl;
    if (player->my_num() == 0)
        print_sender(receiverOutputMatrix.squares[i], t1.squares[i]);
    else
        print_receiver<gf2n>(baseReceiverInput, senderOutputMatrices[0], i);
}

void OTExtensionWithMatrix::print_post_transpose(BitVector& newReceiverInput, int i, int sender)
{
    cout << "post transpose, sender " << sender << endl;
    if (player->my_num() == sender)
    {
        print_receiver<gf2n>(newReceiverInput, receiverOutputMatrix);
    }
    else
    {
        square128 tmp = senderOutputMatrices[0].squares[i];
        tmp ^= baseReceiverInput;
        print_sender(senderOutputMatrices[0].squares[i], tmp);
    }
}

void OTExtensionWithMatrix::print_pre_expand()
{
    cout << "pre expand" << endl;
    if (player->my_num() == 0)
    {
        for (int i = 0; i < 16; i++)
        {
            for (int j = 0; j < 2; j++)
                cout << int128(_mm_loadu_si128((__m128i*)G_sender[i][j].get_seed())) << " ";
            cout << endl;
        }
        cout << endl;
    }
    else
    {
        for (int i = 0; i < 16; i++)
        {
            if (baseReceiverInput.get_bit(i))
            {
                for (int j = 0; j < 33; j++)
                    cout << " ";
            }
            cout << int128(_mm_loadu_si128((__m128i*)G_receiver[i].get_seed())) << endl;
        }
        cout << endl;
    }
}

template void OTExtensionWithMatrix::correlate<gf2n>(int start, int slice,
        BitVector& newReceiverInput, bool useConstantBase, int repeat);
template void OTExtensionWithMatrix::correlate<gfp>(int start, int slice,
        BitVector& newReceiverInput, bool useConstantBase, int repeat);
template void OTExtensionWithMatrix::print_post_correlate<gf2n>(
        BitVector& newReceiverInput, int j, int offset, int sender);
template void OTExtensionWithMatrix::print_post_correlate<gfp>(
        BitVector& newReceiverInput, int j, int offset, int sender);
template void OTExtensionWithMatrix::extend<gf2n>(int nOTs_requested,
        BitVector& newReceiverInput);
template void OTExtensionWithMatrix::extend<gfp>(int nOTs_requested,
        BitVector& newReceiverInput);
template void OTExtensionWithMatrix::expand<gf2n>(int start, int slice);
template void OTExtensionWithMatrix::expand<gfp>(int start, int slice);
template void OTExtensionWithMatrix::expand_transposed<gf2n>();
template void OTExtensionWithMatrix::expand_transposed<gfp>();
template void OTExtensionWithMatrix::reduce_squares(unsigned int nTriples,
        vector<gf2n>& output);
template void OTExtensionWithMatrix::reduce_squares(unsigned int nTriples,
        vector<gfp>& output);
