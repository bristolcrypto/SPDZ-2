// (C) 2018 University of Bristol. See License.txt

/*
 * memcpy.h
 *
 */

#ifndef TOOLS_AVX_MEMCPY_H_
#define TOOLS_AVX_MEMCPY_H_

#include <immintrin.h>
#include <string.h>

inline void avx_memcpy(void* dest, const void* source, size_t length)
{
	__m256i* d = (__m256i*)dest, *s = (__m256i*)source;
#ifdef __AVX__
	while (length >= 32)
	{
		_mm256_storeu_si256(d++, _mm256_loadu_si256(s++));
		length -= 32;
	}
#endif
	__m128i* d2 = (__m128i*)d;
	__m128i* s2 = (__m128i*)s;
	while (length >= 16)
	{
		_mm_storeu_si128(d2++, _mm_loadu_si128(s2++));
		length -= 16;
	}
	if (length)
		memcpy(d2, s2, length);
}

inline void avx_memzero(void* dest, size_t length)
{
	__m256i* d = (__m256i*)dest;
#ifdef __AVX__
	__m256i s = _mm256_setzero_si256();
	while (length >= 32)
	{
		_mm256_storeu_si256(d++, s);
		length -= 32;
	}
#endif
	if (length)
		memset((void*)d, 0, length);
}

#endif /* TOOLS_AVX_MEMCPY_H_ */
