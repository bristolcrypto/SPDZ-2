// (C) 2018 University of Bristol. See License.txt

/*
 * MMO.h
 *
 */

#ifndef TOOLS_MMO_H_
#define TOOLS_MMO_H_

#include "Tools/aes.h"

// Matyas-Meyer-Oseas hashing
class MMO
{
    octet IV[176]  __attribute__((aligned (16)));

    template<int N>
    static void encrypt_and_xor(void* output, const void* input,
            const octet* key);
    template<int N>
    static void encrypt_and_xor(void* output, const void* input,
            const octet* key, const int* indices);

public:
    MMO() { zeroIV(); }
    void zeroIV();
    void setIV(octet key[AES_BLK_SIZE]);
    template <class T>
    void hashOneBlock(octet* output, octet* input);
    template <class T, int N>
    void hashBlockWise(octet* output, octet* input);
    template <class T>
    void outputOneBlock(octet* output);
};

#endif /* TOOLS_MMO_H_ */
