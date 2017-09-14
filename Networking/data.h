// (C) 2017 University of Bristol. See License.txt

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

void encode_length(octet *buff,int len);
int  decode_length(octet *buff);


inline void encode_length(octet *buff,int len)
{
  if (len<0) { throw invalid_length(); }
  buff[0]=len&255;
  buff[1]=(len>>8)&255;
  buff[2]=(len>>16)&255;
  buff[3]=(len>>24)&255;
}

inline int  decode_length(octet *buff)
{
  int len=buff[0]+256*buff[1]+65536*buff[2]+16777216*buff[3];
  if (len<0) { throw invalid_length(); }
  return len;
}

#endif
