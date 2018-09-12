// (C) 2018 University of Bristol. See License.txt

#include "Tools.h"
#include "Math/gf2nlong.h"

void random_seed_commit(octet* seed, TwoPartyPlayer& player, int len)
{
	PRNG G;
	G.ReSeed();
	vector<octetStream> seed_strm(2);
	vector<octetStream> Comm_seed(2);
  	vector<octetStream> Open_seed(2);
	G.get_octetStream(seed_strm[0], len);

	Commit(Comm_seed[0], Open_seed[0], seed_strm[0], player.my_real_num());
	player.send_receive_player(Comm_seed);
	player.send_receive_player(Open_seed);

	memset(seed, 0, len*sizeof(octet));

  	if (!Open(seed_strm[1], Comm_seed[1], Open_seed[1], player.other_player_num()))
    {
    	throw invalid_commitment();
    }
    for (int i = 0; i < len; i++)
	{
		seed[i] = seed_strm[0].get_data()[i] ^ seed_strm[1].get_data()[i];
    }
}

void shiftl128(word x1, word x2, word& res1, word& res2, size_t k)
{
    if (k > 128)
        throw invalid_length();
    if (k >= 64) // shifting a 64-bit integer by more than 63 bits is "undefined"
    {
        x1 = x2;
        x2 = 0;
        shiftl128(x1, x2, res1, res2, k - 64);
    }
    else
    {
        res1 = (x1 << k) | (x2 >> (64-k));
        res2 = (x2 << k);
    }
}

// reduce modulo x^128 + x^7 + x^2 + x + 1
// NB this is incorrect as it bit-reflects the result as required for
// GCM mode
void gfred128(__m128i tmp3, __m128i tmp6, __m128i *res)
{
    __m128i tmp2, tmp4, tmp5, tmp7, tmp8, tmp9;
    tmp7 = _mm_srli_epi32(tmp3, 31);
    tmp8 = _mm_srli_epi32(tmp6, 31);

    tmp3 = _mm_slli_epi32(tmp3, 1);
    tmp6 = _mm_slli_epi32(tmp6, 1);

    tmp9 = _mm_srli_si128(tmp7, 12);
    tmp8 = _mm_slli_si128(tmp8, 4);
    tmp7 = _mm_slli_si128(tmp7, 4);
    tmp3 = _mm_or_si128(tmp3, tmp7);
    tmp6 = _mm_or_si128(tmp6, tmp8);
    tmp6 = _mm_or_si128(tmp6, tmp9);

    tmp7 = _mm_slli_epi32(tmp3, 31);
    tmp8 = _mm_slli_epi32(tmp3, 30);
    tmp9 = _mm_slli_epi32(tmp3, 25);

    tmp7 = _mm_xor_si128(tmp7, tmp8);
    tmp7 = _mm_xor_si128(tmp7, tmp9);
    tmp8 = _mm_srli_si128(tmp7, 4);
    tmp7 = _mm_slli_si128(tmp7, 12);
    tmp3 = _mm_xor_si128(tmp3, tmp7);

    tmp2 = _mm_srli_epi32(tmp3, 1);
    tmp4 = _mm_srli_epi32(tmp3, 2);
    tmp5 = _mm_srli_epi32(tmp3, 7);
    tmp2 = _mm_xor_si128(tmp2, tmp4);
    tmp2 = _mm_xor_si128(tmp2, tmp5);
    tmp2 = _mm_xor_si128(tmp2, tmp8);
    tmp3 = _mm_xor_si128(tmp3, tmp2);

    tmp6 = _mm_xor_si128(tmp6, tmp3);
    *res = tmp6;
}

// Based on Intel's code for GF(2^128) mul, with reduction
void gfmul128 (__m128i a, __m128i b, __m128i *res)
{
    __m128i tmp3, tmp6;
    mul128(a, b, &tmp3, &tmp6);
    // Now do the reduction
    gfred128(tmp3, tmp6, res);
}

string word_to_bytes(const word w)
{
    stringstream ss;
    octet* bytes = (octet*) &w;
    ss << hex;
    for (unsigned int i = 0; i < sizeof(word); i++)
        ss << (int)bytes[i] << " ";
    return ss.str();
}

