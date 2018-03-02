// (C) 2018 University of Bristol. See License.txt

#ifndef _random
#define _random

#include "Tools/octetStream.h"
#include "Tools/sha1.h"
#include "Tools/aes.h"

#define USE_AES

#ifndef USE_AES
  #define PIPELINES   1
  #define SEED_SIZE   HASH_SIZE
  #define RAND_SIZE   HASH_SIZE
#else
  #define PIPELINES   8
  #define SEED_SIZE   AES_BLK_SIZE
  #define RAND_SIZE   (PIPELINES * AES_BLK_SIZE)
#endif


/* This basically defines a randomness expander, if using
 * as a real PRG on an input stream you should first collapse
 * the input stream down to a SEED, say via CBC-MAC (under 0 key)
 * or via a hash
 */

// __attribute__ is needed to get the sse instructions to avoid
//  seg faulting.
     
class PRNG
{
   octet seed[SEED_SIZE]; 
   octet state[RAND_SIZE] __attribute__((aligned (16)));
   octet random[RAND_SIZE] __attribute__((aligned (16)));

   #ifdef USE_AES
     bool useC;

     // Two types of key schedule for the different implementations 
     // of AES
     uint  KeyScheduleC[44];
     octet KeySchedule[176]  __attribute__((aligned (16)));
   #endif

   int cnt;    // How many bytes of the current random value have been used

   void hash(); // Hashes state to random and sets cnt=0
   void next();

   public:

   PRNG();

   // For debugging
   void print_state() const;

   // Set seed from dev/random
   void ReSeed();

   // Set seed from array
   void SetSeed(unsigned char*);
   void SetSeed(PRNG& G);
   void InitSeed();
   
   double get_double();
   bool get_bit() { return get_uchar() & 1; }
   unsigned char get_uchar();
   unsigned int get_uint();
   void get_bigint(bigint& res, int n_bits, bool positive = true);
   void get(bigint& res, int n_bits, bool positive = true);
   void get(int& res, int n_bits, bool positive = true);
   void randomBnd(bigint& res, const bigint& B, bool positive=true);
   bigint randomBnd(const bigint& B, bool positive=true);
   word get_word()
     { word a=get_uint();
       a<<=32; 
       a+=get_uint();
       return a;
     }
   __m128i get_doubleword();
   void get_octetStream(octetStream& ans,int len);
   void get_octets(octet* ans, int len);

   const octet* get_seed() const
     { return seed; }
};

#endif
