// (C) 2018 University of Bristol. See License.txt

/*
 * MMO.cpp
 *
 *
 */

#include "MMO.h"
#include "Math/gf2n.h"
#include "Math/gfp.h"
#include "Math/bigint.h"
#include <unistd.h>


void MMO::zeroIV()
{
    octet key[AES_BLK_SIZE];
    memset(key,0,AES_BLK_SIZE*sizeof(octet));
    setIV(key);
}


void MMO::setIV(octet key[AES_BLK_SIZE])
{
    aes_schedule(IV,key);
}


template<int N>
void MMO::encrypt_and_xor(void* output, const void* input, const octet* key)
{
    __m128i in[N], out[N];
    avx_memcpy(in, input, sizeof(in));
    ecb_aes_128_encrypt<N>(out, in, key);
    for (int i = 0; i < N; i++)
        out[i] = _mm_xor_si128(out[i], in[i]);
    avx_memcpy(output, out, sizeof(out));
}

template<int N>
void MMO::encrypt_and_xor(void* output, const void* input, const octet* key,
        const int* indices)
{
    __m128i in[N], out[N];
    for (int i = 0; i < N; i++)
        in[i] = _mm_loadu_si128(((__m128i*)input) + indices[i]);
    encrypt_and_xor<N>(out, in, key);
    for (int i = 0; i < N; i++)
        _mm_storeu_si128(((__m128i*)output) + indices[i], out[i]);
}

template <>
void MMO::hashOneBlock<gf2n>(octet* output, octet* input)
{
    encrypt_and_xor<1>(output, input, IV);
}


template <>
void MMO::hashOneBlock<gfp>(octet* output, octet* input)
{
    encrypt_and_xor<1>(output, input, IV);
    while (mpn_cmp((mp_limb_t*)output, gfp::get_ZpD().get_prA(), gfp::t()) >= 0)
        encrypt_and_xor<1>(output, output, IV);
}

template <>
void MMO::hashBlockWise<gf2n,128>(octet* output, octet* input)
{
    for (int i = 0; i < 16; i++)
        encrypt_and_xor<8>(&((__m128i*)output)[i*8], &((__m128i*)input)[i*8], IV);
}

template <>
void MMO::hashBlockWise<gfp,128>(octet* output, octet* input)
{
    for (int i = 0; i < 16; i++)
    {
        __m128i* in = &((__m128i*)input)[i*8];
        __m128i* out = &((__m128i*)output)[i*8];
        encrypt_and_xor<8>(out, in, IV);
        int left = 8;
        int indices[8] = {0, 1, 2, 3, 4, 5, 6, 7};
        while (left)
        {
            int now_left = 0;
            for (int j = 0; j < left; j++)
                if (mpn_cmp((mp_limb_t*)&out[indices[j]], gfp::get_ZpD().get_prA(), gfp::t()) >= 0)
                {
                    indices[now_left] = indices[j];
                    now_left++;
                }
            left = now_left;

            // and now my favorite hack
            switch (left) {
            case 8:
                ecb_aes_128_encrypt<8>(out, out, IV, indices);
                break;
            case 7:
                ecb_aes_128_encrypt<7>(out, out, IV, indices);
                break;
            case 6:
                ecb_aes_128_encrypt<6>(out, out, IV, indices);
                break;
            case 5:
                ecb_aes_128_encrypt<5>(out, out, IV, indices);
                break;
            case 4:
                ecb_aes_128_encrypt<4>(out, out, IV, indices);
                break;
            case 3:
                ecb_aes_128_encrypt<3>(out, out, IV, indices);
                break;
            case 2:
                ecb_aes_128_encrypt<2>(out, out, IV, indices);
                break;
            case 1:
                ecb_aes_128_encrypt<1>(out, out, IV, indices);
                break;
            default:
                break;
            }
        }
    }
}
