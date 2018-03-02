// (C) 2018 University of Bristol. See License.txt

#ifndef _Data
#define _Data

#include <string.h>

#include "Exceptions/Exceptions.h"


typedef unsigned char octet;

// Assumes word is a 64 bit value
#ifdef WIN32
  typedef unsigned __int64 word;
#else
  typedef unsigned long word;
#endif

#define BROADCAST 0
#define ROUTE     1
#define TERMINATE 2
#define GO        3


inline void encode_length(octet *buff, size_t len, size_t n_bytes)
{
    if (n_bytes > 8)
        throw invalid_length("length field cannot be more than 64 bits");
    if (n_bytes < 8 && (len > (1ULL << (8 * n_bytes))))
        throw invalid_length("length too large for length field");
    for (size_t i = 0; i < n_bytes; i++)
        buff[i] = len >> (8 * i);
}

inline size_t decode_length(octet *buff, size_t n_bytes)
{
    size_t len = 0;
    for (size_t i = 0; i < n_bytes; i++)
    {
        len += (size_t) buff[i] << (8 * i);
    }
    return len;
}

#endif
