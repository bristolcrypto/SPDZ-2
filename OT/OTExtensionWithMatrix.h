// (C) 2018 University of Bristol. See License.txt

/*
 * OTExtensionWithMatrix.h
 *
 */

#ifndef OT_OTEXTENSIONWITHMATRIX_H_
#define OT_OTEXTENSIONWITHMATRIX_H_

#include "OTExtension.h"
#include "BitMatrix.h"
#include "Math/gf2n.h"

class OTExtensionWithMatrix : public OTExtension
{
public:
    vector<BitMatrix> senderOutputMatrices;
    BitMatrix receiverOutputMatrix;
    BitMatrix t1, u;
    PRNG G;

    OTExtensionWithMatrix(int nbaseOTs, int baseLength,
                int nloops, int nsubloops,
                TwoPartyPlayer* player,
                BitVector& baseReceiverInput,
                vector< vector<BitVector> >& baseSenderInput,
                vector<BitVector>& baseReceiverOutput,
                OT_ROLE role=BOTH,
                bool passive=false)
    : OTExtension(nbaseOTs, baseLength, nloops, nsubloops, player, baseReceiverInput,
            baseSenderInput, baseReceiverOutput, role, passive) {
      G.ReSeed();
    }

    void seed(vector<BitMatrix>& baseSenderInput,
            BitMatrix& baseReceiverOutput);
    void transfer(int nOTs, const BitVector& receiverInput);
    void resize(int nOTs);
    template <class T>
    void extend(int nOTs, BitVector& newReceiverInput);
    template <class T>
    void expand(int start, int slice);
    template <class T>
    void expand_transposed();
    template <class T>
    void correlate(int start, int slice, BitVector& newReceiverInput, bool useConstantBase, int repeat = 1);
    void transpose(int start, int slice);
    void setup_for_correlation(vector<BitMatrix>& baseSenderOutputs, BitMatrix& baseReceiverOutput);
    template <class T>
    void reduce_squares(unsigned int nTriples, vector<T>& output);

    void print(BitVector& newReceiverInput, int i = 0);
    template <class T>
    void print_receiver(BitVector& newReceiverInput, BitMatrix& matrix, int i = 0, int offset = 0);
    void print_sender(square128& square0, square128& square);
    template <class T>
    void print_post_correlate(BitVector& newReceiverInput, int i = 0, int offset = 0, int sender = 0);
    void print_pre_correlate(int i = 0);
    void print_post_transpose(BitVector& newReceiverInput, int i = 0,  int sender = 0);
    void print_pre_expand();

    octet* get_receiver_output(int i);
    octet* get_sender_output(int choice, int i);

protected:
    template <class T>
    void hash_outputs(int nOTs);
};

#endif /* OT_OTEXTENSIONWITHMATRIX_H_ */
