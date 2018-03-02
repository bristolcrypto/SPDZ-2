// (C) 2018 University of Bristol. See License.txt

#include "OTExtension.h"

#include "OT/Tools.h"
#include "Math/gf2n.h"
#include "Tools/aes.h"
#include "Tools/MMO.h"
#include <wmmintrin.h>
#include <emmintrin.h>


word TRANSPOSE_MASKS128[7][2] = {
    { 0x0000000000000000, 0xFFFFFFFFFFFFFFFF },
    { 0x00000000FFFFFFFF, 0x00000000FFFFFFFF },
    { 0x0000FFFF0000FFFF, 0x0000FFFF0000FFFF },
    { 0x00FF00FF00FF00FF, 0x00FF00FF00FF00FF },
    { 0x0F0F0F0F0F0F0F0F, 0x0F0F0F0F0F0F0F0F },
    { 0x3333333333333333, 0x3333333333333333 },
    { 0x5555555555555555, 0x5555555555555555 }
};

string word_to_str(word a)
{
    stringstream ss;
    ss << hex;
    for(int i = 0;i < 8; i++)
        ss << ((a >> (i*8)) & 255) << " ";
    return ss.str();
}

// Transpose 16x16 matrix starting at bv[x][y] in-place using SSE2
void sse_transpose16(vector<BitVector>& bv, int x, int y)
{
    __m128i input[2];
    // 16x16 in two halves, 128 bits each
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 16; j++)
            ((octet*)&input[i])[j] = bv[x+j].get_byte(y / 8 + i);

    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 8; j++)
        {
            int output = _mm_movemask_epi8(input[i]);
            input[i] = _mm_slli_epi64(input[i], 1);

            for (int k = 0; k < 2; k++)
                // _mm_movemask_epi8 uses most significant bit, hence +7-j
                bv[x+8*i+7-j].set_byte(y / 8 + k, ((octet*)&output)[k]);
        }
}

/*
 * Transpose 128x128 bit-matrix using Eklundh's algorithm
 *
 * Input is in input[i] [ bits <offset> to <offset+127> ], i = 0, ..., 127
 * Output is in output[i + offset] (entire 128-bit vector), i = 0, ..., 127.
 *
 * Transposes 128-bit vectors in little-endian format.
 */
//void eklundh_transpose64(vector<word>& output, const vector<Big_Keys>& input, int offset)
void eklundh_transpose128(vector<BitVector>& output, const vector<BitVector>& input,
    int offset)
{
    int width = 64;
    int logn = 7, nswaps = 1;

#ifdef TRANSPOSE_DEBUG
    stringstream input_ss[128];
    stringstream output_ss[128];
#endif
    // first copy input to output
    for (int i = 0; i < 128; i++)
    {
        //output[i + offset*64] = input[i].get(offset);
        output[i + offset].set_word(0, input[i].get_word(offset/64));
        output[i + offset].set_word(1, input[i].get_word(offset/64 + 1));

#ifdef TRANSPOSE_DEBUG
        for (int j = 0; j < 128; j++)
        {
            input_ss[j] << input[i].get_bit(offset + j);
        }
#endif
    }

    // now transpose output in-place
    for (int i = 0; i < logn; i++)
    {
        word mask1 = TRANSPOSE_MASKS128[i][1], mask2 = TRANSPOSE_MASKS128[i][0];
        word inv_mask1 = ~mask1, inv_mask2 = ~mask2;

        if (width == 8)
        {
            for (int j = 0; j < 8; j++)
                for (int k = 0; k < 8; k++)
                    sse_transpose16(output, offset + 16 * j, 16 * k);
            break;
        }
        else
        // for width >= 64, shift is undefined so treat as a special case
        // (and avoid branching in inner loop)
        if (width < 64)
        {
            for (int j = 0; j < nswaps; j++)
            {
                for (int k = 0; k < width; k++)
                {
                    int i1 = k + 2*width*j;
                    int i2 = k + width + 2*width*j;

                    // t1 is lower 64 bits, t2 is upper 64 bits
                    // (remember we're transposing in little-endian format)
                    word t1 = output[i1 + offset].get_word(0);
                    word t2 = output[i1 + offset].get_word(1);

                    word tt1 = output[i2 + offset].get_word(0);
                    word tt2 = output[i2 + offset].get_word(1);

                    // swap operations due to little endian-ness
                    output[i1 + offset].set_word(0, (t1 & mask1) ^
                        ((tt1 & mask1) << width));
                    output[i1 + offset].set_word(1, (t2 & mask2) ^
                        ((tt2 & mask2) << width) ^
                        ((tt1 & mask1) >> (64 - width)));

                    output[i2 + offset].set_word(0, (tt1 & inv_mask1) ^
                        ((t1 & inv_mask1) >> width) ^
                        ((t2 & inv_mask2)) << (64 - width));
                    output[i2 + offset].set_word(1, (tt2 & inv_mask2) ^
                        ((t2 & inv_mask2) >> width));
                }
            }
        }
        else
        {
            for (int j = 0; j < nswaps; j++)
            {
                for (int k = 0; k < width; k++)
                {
                    int i1 = k + 2*width*j;
                    int i2 = k + width + 2*width*j;

                    // t1 is lower 64 bits, t2 is upper 64 bits
                    // (remember we're transposing in little-endian format)
                    word t1 = output[i1 + offset].get_word(0);
                    word t2 = output[i1 + offset].get_word(1);

                    word tt1 = output[i2 + offset].get_word(0);
                    word tt2 = output[i2 + offset].get_word(1);

                    output[i1 + offset].set_word(0, (t1 & mask1));
                    output[i1 + offset].set_word(1, (t2 & mask2) ^
                        ((tt1 & mask1) >> (64 - width)));

                    output[i2 + offset].set_word(0, (tt1 & inv_mask1) ^
                        ((t2 & inv_mask2)) << (64 - width));
                    output[i2 + offset].set_word(1, (tt2 & inv_mask2));
                }
            }
        }
        nswaps *= 2;
        width /= 2;
    }
#ifdef TRANSPOSE_DEBUG
    for (int i = 0; i < 128; i++)
    {
        for (int j = 0; j < 128; j++)
        {
            output_ss[j] << output[offset + j].get_bit(i);
        }
    }
    for (int i = 0; i < 128; i++)
    {
        if (output_ss[i].str().compare(input_ss[i].str()) != 0)
        {
            cerr << "String " << i << " failed. offset = " << offset << endl;
            cerr << input_ss[i].str() << endl;
            cerr << output_ss[i].str() << endl;
            exit(1);
        }
    }
    cout << "\ttranspose with offset " << offset << " ok\n";
#endif
}


// get bit, starting from MSB as bit 0
int get_bit(word x, int b)
{
    return (x >> (63 - b)) & 1;
}
int get_bit128(word x1, word x2, int b)
{
    if (b < 64)
    {
        return (x1 >> (b - 64)) & 1;
    }
    else
    {
        return (x2 >> b) & 1;
    }
}

void naive_transpose128(vector<BitVector>& output, const vector<BitVector>& input,
    int offset)
{
    for (int i = 0; i < 128; i++)
    {
        // NB: words are read from input in big-endian format
        word w1 = input[i].get_word(offset/64);
        word w2 = input[i].get_word(offset/64 + 1);

        for (int j = 0; j < 128; j++)
        {
            //output[j + offset].set_bit(i, input[i].get_bit(j + offset));

            if (j < 64)
                output[j + offset].set_bit(i, (w1 >> j) & 1);
            else
                output[j + offset].set_bit(i, w2 >> (j-64) & 1);
        }
    }
}

void transpose64(
    vector<BitVector>::iterator& output_it,
    vector<BitVector>::iterator& input_it)
{
    for (int i = 0; i < 64; i++)
    {
        for (int j = 0; j < 64; j++)
        {
            (output_it + j)->set_bit(i, (input_it + i)->get_bit(j));
        }
    }
}

// Naive 64x64 bit matrix transpose
void naive_transpose64(vector<BitVector>& output, const vector<BitVector>& input,
    int xoffset, int yoffset)
{
    int word_size = 64;

    for (int i = 0; i < word_size; i++)
    {
        word w = input[i + yoffset].get_word(xoffset);
        for (int j = 0; j < word_size; j++)
        {
            //cout << j + xoffset*word_size << ", " << yoffset/word_size << endl;
            //int wbit = (((w >> j) & 1) << i); cout << "wbit " << wbit << endl;

            // set i-th bit of output to j-th bit of w
            // scale yoffset by 64 since we're selecting words from the BitVector
            word tmp = output[j + xoffset*word_size].get_word(yoffset/word_size);
            output[j + xoffset*word_size].set_word(yoffset/word_size, tmp ^ ((w >> j) & 1) << i);
            // set i-th bit of output to j-th bit of w
            //output[j + offset*word_size] ^= ((w >> j) & 1) << i;
        }
    }
}

void OTExtension::transfer(int nOTs,
        const BitVector& receiverInput)
{
#ifdef OTEXT_TIMER
    timeval totalstartv, totalendv;
    gettimeofday(&totalstartv, NULL);
#endif
    cout << "\tDoing " << nOTs << " extended OTs as " << role_to_str(ot_role) << endl;
    // add k + s to account for discarding k OTs
    nOTs += 2 * 128;
    if (nOTs % nbaseOTs != 0)
        throw invalid_length(); //"nOTs must be a multiple of nbaseOTs\n");
    if (nOTs == 0)
        return;

    vector<BitVector> t0(nbaseOTs, BitVector(nOTs)), tmp(nbaseOTs, BitVector(nOTs)), t1(nbaseOTs, BitVector(nOTs));
    BitVector u(nOTs);
    senderOutput.resize(2, vector<BitVector>(nOTs, BitVector(nbaseOTs)));
    // resize to account for extra k OTs that are discarded
    PRNG G;
    G.ReSeed();
    BitVector newReceiverInput(nOTs);
    for (unsigned int i = 0; i < receiverInput.size_bytes(); i++)
    {
        newReceiverInput.set_byte(i, receiverInput.get_byte(i));
    }

    //BitVector newReceiverInput(receiverInput);
    newReceiverInput.resize(nOTs);

    receiverOutput.resize(nOTs, BitVector(nbaseOTs));
    
    for (int loop = 0; loop < nloops; loop++)
    {
        vector<octetStream> os(2), tmp_os(2);

        // randomize last 128 + 128 bits that will be discarded
        for (int i = 0; i < 4; i++)
            newReceiverInput.set_word(nOTs/64 - i, G.get_word());

        // expand with PRG and create correlation
        if (ot_role & RECEIVER)
        {
            for (int i = 0; i < nbaseOTs; i++)
            {
                t0[i].randomize(G_sender[i][0]);
                t1[i].randomize(G_sender[i][1]);
                tmp[i].assign(t1[i]);

                tmp[i].add(t0[i]);
                tmp[i].add(newReceiverInput);
                tmp[i].pack(os[0]);
                /*cout << "t0: " << t0[i].str() << endl;
            cout << "t1: " << t1[i].str() << endl;
            cout << "Sending tmp: " << tmp[i].str() << endl;*/
            }
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
            for (int i = 0; i < nbaseOTs; i++)
            {
                // randomize base receiver output
                tmp[i].randomize(G_receiver[i]);

                // u = t0 + t1 + x
                u.unpack(os[1]);

                if (baseReceiverInput.get_bit(i) == 1)
                {
                    // now tmp is q[i] = t0[i] + Delta[i] * x
                    tmp[i].add(u);
                }
            }
        }
#ifdef OTEXT_TIMER
        gettimeofday(&commst2, NULL);
        double commstime = timeval_diff(&commst1, &commst2);
        cout << "\t\tCommunication took time " << commstime/1000000 << endl << flush;
        times["Communication"] += timeval_diff(&commst1, &commst2);
#endif

        // transpose t0[i] onto receiverOutput and tmp (q[i]) onto senderOutput[i][0]

        // stupid transpose
        /*for (int j = 0; j < nOTs; j++)
    {
        for (int i = 0; i < nbaseOTs; i++)
        {
            senderOutput[0][j].set_bit(i, t0[i].get_bit(j));
            receiverOutput[j].set_bit(i, tmp[i].get_bit(j));
        }
    }*/
        cout << "Starting matrix transpose\n" << flush << endl;
#ifdef OTEXT_TIMER
        timeval transt1, transt2;
        gettimeofday(&transt1, NULL);
#endif
        // transpose in 128-bit chunks with Eklundh's algorithm
        for (int i = 0; i < nOTs / 128; i++)
        {
            if (ot_role & RECEIVER)
            {
                eklundh_transpose128(receiverOutput, t0, i*128);
                //naive_transpose128(receiverOutput, t0, i*128);
            }
            if (ot_role & SENDER)
            {
                eklundh_transpose128(senderOutput[0], tmp, i*128);
                //naive_transpose128(senderOutput[0], tmp, i*128);
            }
        }

#ifdef OTEXT_TIMER
        gettimeofday(&transt2, NULL);
        double transtime = timeval_diff(&transt1, &transt2);
        cout << "\t\tMatrix transpose took time " << transtime/1000000 << endl << flush;
        times["Matrix transpose"] += timeval_diff(&transt1, &transt2);
#endif

#ifdef OTEXT_DEBUG
        // verify correctness of the OTs
        // i.e. senderOutput[0][i] + x_i * Delta = receiverOutput[i]
        // (where Delta = baseReceiverOutput)
        BitVector tmp_vector1(nbaseOTs), tmp_vector2(nOTs);//nbaseOTs);
        cout << "\tVerifying OT extensions (debugging)\n";
        for (int i = 0; i < nOTs; i++)
        {
            os[0].reset_write_head();
            os[1].reset_write_head();

            if (ot_role & RECEIVER)
            {
                // send t0 and x over
                receiverOutput[i].pack(os[0]);
                //t0[i].pack(os[0]);
                newReceiverInput.pack(os[0]);
            }
            send_if_ot_receiver(player, os, ot_role);

            if (ot_role & SENDER)
            {
                tmp_vector1.unpack(os[1]);
                tmp_vector2.unpack(os[1]);

                // if x_i = 1, add Delta
                if (tmp_vector2.get_bit(i) == 1)
                {
                    tmp_vector1.add(baseReceiverInput);
                }
                if (!tmp_vector1.equals(senderOutput[0][i]))
                {
                    cerr << "Incorrect OT at " << i << "\n";
                    exit(1);
                }
            }
        }
        cout << "Correlated OTs all OK\n";
#endif

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

        hash_outputs(nOTs, receiverOutput);
#ifdef OTEXT_TIMER
        gettimeofday(&totalendv, NULL);
        elapsed = timeval_diff(&totalstartv, &totalendv);
        cout << "\t\tTotal thread time: " << elapsed/1000000 << endl << flush;
#endif

#ifdef OTEXT_DEBUG
        // verify correctness of the random OTs
        // i.e. senderOutput[0][i] + x_i * Delta = receiverOutput[i]
        // (where Delta = baseReceiverOutput)
        cout << "Verifying random OTs (debugging)\n";
        for (int i = 0; i < nOTs; i++)
        {
            os[0].reset_write_head();
            os[1].reset_write_head();

            if (ot_role & RECEIVER)
            {
                // send receiver's input/output over
                receiverOutput[i].pack(os[0]);
                newReceiverInput.pack(os[0]);
            }
            send_if_ot_receiver(player, os, ot_role);
            //player->send_receive_player(os);
            if (ot_role & SENDER)
            {
                tmp_vector1.unpack(os[1]);
                tmp_vector2.unpack(os[1]);

                // if x_i = 1, comp with sender output[1]
                if ((tmp_vector2.get_bit(i) == 1))
                {
                    if (!tmp_vector1.equals(senderOutput[1][i]))
                    {
                        cerr << "Incorrect OT\n";
                        exit(1);
                    }
                }
                // else should be sender output[0]
                else if (!tmp_vector1.equals(senderOutput[0][i]))
                {
                    cerr << "Incorrect OT\n";
                    exit(1);
                }
            }
        }
        cout << "Random OTs all OK\n";
#endif
    }

#ifdef OTEXT_TIMER
    gettimeofday(&totalendv, NULL);
    times["Total thread"] +=  timeval_diff(&totalstartv, &totalendv);
#endif

    receiverOutput.resize(nOTs - 2 * 128);
    senderOutput[0].resize(nOTs - 2 * 128);
    senderOutput[1].resize(nOTs - 2 * 128);
}

/*
 * Hash outputs to make into random OT
 */
void OTExtension::hash_outputs(int nOTs, vector<BitVector>& receiverOutput)
{
    cout << "Hashing... " << flush;
    octetStream os, h_os(HASH_SIZE);
    BitVector tmp(nbaseOTs);
    MMO mmo;
#ifdef OTEXT_TIMER
    timeval startv, endv;
    gettimeofday(&startv, NULL);
#endif

    for (int i = 0; i < nOTs; i++)
    {
        if (ot_role & SENDER)
        {
            tmp.add(senderOutput[0][i], baseReceiverInput);

            if (senderOutput[0][i].size() == 128)
            {
                mmo.hashOneBlock<gf2n>(senderOutput[0][i].get_ptr(), senderOutput[0][i].get_ptr());
                mmo.hashOneBlock<gf2n>(senderOutput[1][i].get_ptr(), tmp.get_ptr());
            }
            else
            {
                os.reset_write_head();
                h_os.reset_write_head();

                senderOutput[0][i].pack(os);
                os.hash(h_os);
                senderOutput[0][i].unpack(h_os);

                os.reset_write_head();
                h_os.reset_write_head();

                tmp.pack(os);
                os.hash(h_os);
                senderOutput[1][i].unpack(h_os);
            }
        }
        if (ot_role & RECEIVER)
        {
            if (receiverOutput[i].size() == 128)
                mmo.hashOneBlock<gf2n>(receiverOutput[i].get_ptr(), receiverOutput[i].get_ptr());
            else
            {
                os.reset_write_head();
                h_os.reset_write_head();

                receiverOutput[i].pack(os);
                os.hash(h_os);
                receiverOutput[i].unpack(h_os);
            }
        }
    }
    cout << "done.\n";
#ifdef OTEXT_TIMER
    gettimeofday(&endv, NULL);
    double elapsed = timeval_diff(&startv, &endv);
    cout << "\t\tOT ext hashing took time " << elapsed/1000000 << endl << flush;
    times["Hashing"] += timeval_diff(&startv, &endv);
#endif
}


// test if a == b
int eq_m128i(__m128i a, __m128i b)
{
    __m128i vcmp = _mm_cmpeq_epi8(a, b);
    uint16_t vmask = _mm_movemask_epi8(vcmp);
    return (vmask == 0xffff);
}

void random_m128i(PRNG& G, __m128i *r)
{
    BitVector rv(128);
    rv.randomize(G);
    *r = _mm_load_si128((__m128i*)&(rv.get_ptr()[0]));
}

void test_mul()
{
    cout << "Testing GF(2^128) multiplication\n";
    __m128i t1, t2, t3, t4, t5, t6, t7, t8;
    PRNG G;
    G.ReSeed();
    BitVector r(128);
    for (int i = 0; i < 1000; i++)
    {
        random_m128i(G, &t1);
        random_m128i(G, &t2);
        // test commutativity
        gfmul128(t1, t2, &t3);
        gfmul128(t2, t1, &t4);
        if (!eq_m128i(t3, t4))
        {
            cerr << "Incorrect multiplication:\n";
            cerr << "t1 * t2 = " << __m128i_toString<octet>(t3) << endl;
            cerr << "t2 * t1 = " << __m128i_toString<octet>(t4) << endl;
        }
        // test distributivity: t1*t3 + t2*t3 = (t1 + t2) * t3
        random_m128i(G, &t1);
        random_m128i(G, &t2);
        random_m128i(G, &t3);
        gfmul128(t1, t3, &t4);
        gfmul128(t2, t3, &t5);
        t6 = _mm_xor_si128(t4, t5);

        t7 = _mm_xor_si128(t1, t2);
        gfmul128(t7, t3, &t8);
        if (!eq_m128i(t6, t8))
        {
            cerr << "Incorrect multiplication:\n";
            cerr << "t1 * t3 + t2 * t3 = " << __m128i_toString<octet>(t6) << endl;
            cerr << "(t1 + t2) * t3 = " << __m128i_toString<octet>(t8) << endl;
        }
    }
    t1 = _mm_set_epi32(0, 0, 0, 03);
    t2 = _mm_set_epi32(0, 0, 0, 11);
    //gfmul128(t1, t2, &t3);
    mul128(t1, t2, &t3, &t4);
    cout << "t1 = " << __m128i_toString<octet>(t1) << endl;
    cout << "t2 = " << __m128i_toString<octet>(t2) << endl;
    cout << "t3 = " << __m128i_toString<octet>(t3) << endl;
    cout << "t4 = " << __m128i_toString<octet>(t4) << endl;

    uint64_t cc[] __attribute__((aligned (16))) = { 0,0 };
    _mm_store_si128((__m128i*)cc, t1);
    word t1w = cc[0];
    _mm_store_si128((__m128i*)cc, t2);
    word t2w = cc[0];

    cout << "t1w = " << t1w << endl;
    cout << "t1 = " << word_to_bytes(t1w) << endl;
    cout << "t2 = " << word_to_bytes(t2w) << endl;
    cout << "t1 * t2 = " << word_to_bytes(t1w*t2w) << endl;
}



void OTExtension::check_correlation(int nOTs,
    const BitVector& receiverInput)
{
    //cout << "\tStarting correlation check\n" << flush;
#ifdef OTEXT_TIMER
    timeval startv, endv;
    gettimeofday(&startv, NULL);
#endif
    if (nbaseOTs != 128)
    {
        cerr << "Correlation check not implemented for length != 128\n";
        throw not_implemented();
    }
    PRNG G;
    octet* seed = new octet[SEED_SIZE];
    random_seed_commit(seed, *player, SEED_SIZE);
#ifdef OTEXT_TIMER
    gettimeofday(&endv, NULL);
    double elapsed = timeval_diff(&startv, &endv);
    cout << "\t\tCommitment for seed took time " << elapsed/1000000 << endl << flush;
    times["Commitment for seed"] += timeval_diff(&startv, &endv);
    gettimeofday(&startv, NULL);
#endif
    G.SetSeed(seed);
    delete[] seed;
    vector<octetStream> os(2);

    if (!Check_CPU_support_AES())
    {
        cerr << "Not implemented GF(2^128) multiplication in C\n";
        throw not_implemented();
    }

    __m128i Delta, x128i;
    Delta = _mm_load_si128((__m128i*)&(baseReceiverInput.get_ptr()[0]));

    BitVector chi(nbaseOTs);
    BitVector x(nbaseOTs);
    __m128i t = _mm_setzero_si128();
    __m128i q = _mm_setzero_si128();
    __m128i t2 = _mm_setzero_si128();
    __m128i q2 = _mm_setzero_si128();
    __m128i chii, ti, qi, ti2, qi2;
    x128i = _mm_setzero_si128();

    for (int i = 0; i < nOTs; i++)
    {
//        chi.randomize(G);
//        chii = _mm_load_si128((__m128i*)&(chi.get_ptr()[0]));
        chii = G.get_doubleword();

        if (ot_role & RECEIVER)
        {
            if (receiverInput.get_bit(i) == 1)
            {
                x128i = _mm_xor_si128(x128i, chii);
            }
            ti = _mm_loadu_si128((__m128i*)get_receiver_output(i));
            // multiply over polynomial ring to avoid reduction
            mul128(ti, chii, &ti, &ti2);
            t = _mm_xor_si128(t, ti);
            t2 = _mm_xor_si128(t2, ti2);
        }
        if (ot_role & SENDER)
        {
            qi = _mm_loadu_si128((__m128i*)(get_sender_output(0, i)));
            mul128(qi, chii, &qi, &qi2);
            q = _mm_xor_si128(q, qi);
            q2 = _mm_xor_si128(q2, qi2);
        }
    }
#ifdef OTEXT_DEBUG
    if (ot_role & RECEIVER)
    {
        cout << "\tSending x,t\n";
        cout << "\tsend x = " << __m128i_toString<octet>(x128i) << endl;
        cout << "\tsend t = " << __m128i_toString<octet>(t) << endl;
        cout << "\tsend t2 = " << __m128i_toString<octet>(t2) << endl;
    }
#endif
    check_iteration(Delta, q, q2, t, t2, x128i);
#ifdef OTEXT_TIMER
    gettimeofday(&endv, NULL);
    elapsed = timeval_diff(&startv, &endv);
    cout << "\t\tChecking correlation took time " << elapsed/1000000 << endl << flush;
    times["Checking correlation"] += timeval_diff(&startv, &endv);
#endif
}

void OTExtension::check_iteration(__m128i delta, __m128i q, __m128i q2,
    __m128i t, __m128i t2, __m128i x)
{
    vector<octetStream> os(2);
    // send x, t;
    __m128i received_t, received_t2, received_x, tmp1, tmp2;
    if (ot_role & RECEIVER)
    {
        os[0].append((octet*)&x, sizeof(x));
        os[0].append((octet*)&t, sizeof(t));
        os[0].append((octet*)&t2, sizeof(t2));
    }
    send_if_ot_receiver(player, os, ot_role);

    if (ot_role & SENDER)
    {
        os[1].consume((octet*)&received_x, sizeof(received_x));
        os[1].consume((octet*)&received_t, sizeof(received_t));
        os[1].consume((octet*)&received_t2, sizeof(received_t2));

        // check t = x * Delta + q
        //gfmul128(received_x, delta, &tmp1);
        mul128(received_x, delta, &tmp1, &tmp2);
        tmp1 = _mm_xor_si128(tmp1, q);
        tmp2 = _mm_xor_si128(tmp2, q2);

        if (eq_m128i(tmp1, received_t) && eq_m128i(tmp2, received_t2))
        {
            //cout << "\tCheck passed\n";
        }
        else
        {
            cerr << "Correlation check failed\n";
            cout << "rec t = " << __m128i_toString<octet>(received_t) << endl;
            cout << "tmp1  = " << __m128i_toString<octet>(tmp1) << endl;
            cout << "q  = " << __m128i_toString<octet>(q) << endl;
            exit(1);
        }
    }
}


octet* OTExtension::get_receiver_output(int i)
{
    return receiverOutput[i].get_ptr();
}

octet* OTExtension::get_sender_output(int choice, int i)
{
    return senderOutput[choice][i].get_ptr();
}
