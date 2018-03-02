// (C) 2018 University of Bristol. See License.txt

/*
 * BitMatrix.h
 *
 */

#ifndef OT_BITMATRIX_H_
#define OT_BITMATRIX_H_

#include <vector>
#include <emmintrin.h>

#include "BitVector.h"
#include "Tools/random.h"
#include "Tools/MMO.h"
#include "Math/gf2nlong.h"

using namespace std;

union square128 {
    __m128i rows[128];
    octet bytes[128][16];
    int16_t doublebytes[128][8];
    int32_t words[128][4];

    bool get_bit(int x, int y)
    { return (bytes[x][y/8] >> (y % 8)) & 1; }

    void set_zero();

    square128& operator^=(square128& other);
    square128& operator^=(__m128i* other);
    square128& operator^=(BitVector& other);
    bool operator==(square128& other);

    template <class T>
    square128& add(square128& other);
    template <class T>
    square128& sub(square128& other);
    template <class T>
    square128& rsub(square128& other);
    template <class T>
    square128& sub(__m128i* other);

    void randomize(PRNG& G);
    template <class T>
    void randomize(int row, PRNG& G);
    template <class T>
    void conditional_add(BitVector& conditions, square128& other, int offset);
    void transpose();
    template <class T>
    void hash_row_wise(MMO& mmo, square128& input);
    template <class T>
    void to(T& result);

    void check_transpose(square128& dual, int i, int k);
    void print(int i, int k);
    void print();

    // Pack and unpack in native format
    //   i.e. Dont care about conversion to human readable form
    void pack(octetStream& o) const;
    void unpack(octetStream& o);
};

class BitMatrixSlice;

class BitMatrix
{
public:
    vector<square128> squares;

    BitMatrix() {}
    BitMatrix(int length);
    void resize(int length);
    int size();

    template <class T>
    BitMatrix& add(BitMatrix& other);
    template <class T>
    BitMatrix& sub(BitMatrix& other);
    template <class T>
    BitMatrix& rsub(BitMatrixSlice& other);
    template <class T>
    BitMatrix& sub(BitVector& other);
    bool operator==(BitMatrix& other);
    bool operator!=(BitMatrix& other);

    void randomize(PRNG& G);
    void randomize(int row, PRNG& G);
    void transpose();

    void check_transpose(BitMatrix& dual);
    void print_side_by_side(BitMatrix& other);
    void print_conditional(BitVector& conditions);

    // Pack and unpack in native format
    //   i.e. Dont care about conversion to human readable form
    void pack(octetStream& o) const;
    void unpack(octetStream& o);

    void to(vector<BitVector>& output);
};

class BitMatrixSlice
{
    friend class BitMatrix;

    BitMatrix& bm;
    size_t start, end;

public:
    BitMatrixSlice(BitMatrix& bm, size_t start, size_t size);

    template <class T>
    BitMatrixSlice& rsub(BitMatrixSlice& other);
    template <class T>
    BitMatrixSlice& add(BitVector& other, int repeat = 1);

    template <class T>
    void randomize(int row, PRNG& G);
    template <class T>
    void conditional_add(BitVector& conditions, BitMatrix& other, bool useOffset = false);
    void transpose();

    template <class T>
    void print();

    // Pack and unpack in native format
    //   i.e. Dont care about conversion to human readable form
    void pack(octetStream& o) const;
    void unpack(octetStream& o);
};

#endif /* OT_BITMATRIX_H_ */
