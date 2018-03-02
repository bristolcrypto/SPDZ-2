// (C) 2018 University of Bristol. See License.txt


#include "Tools/random.h"
#include "Math/bigint.h"
#include <stdio.h>
#include <sodium.h>

#include <iostream>
using namespace std;


PRNG::PRNG() : cnt(0)
{
  #ifdef USE_AES
    useC=(Check_CPU_support_AES()==0);
  #endif
  
}

void PRNG::ReSeed()
{
  randombytes_buf(seed, SEED_SIZE);
  InitSeed();
}


void PRNG::SetSeed(octet* inp)
{
  memcpy(seed,inp,SEED_SIZE*sizeof(octet));
  InitSeed();
}

void PRNG::SetSeed(PRNG& G)
{
  octet tmp[SEED_SIZE];
  G.get_octets(tmp, sizeof(tmp));
  SetSeed(tmp);
}

void PRNG::InitSeed()
{
  #ifdef USE_AES
     if (useC)
        { aes_schedule(KeyScheduleC,seed); }
     else
        { aes_schedule(KeySchedule,seed); }
     memset(state,0,RAND_SIZE*sizeof(octet));
     for (int i = 0; i < PIPELINES; i++)
         state[i*AES_BLK_SIZE] = i;
  #else
     memcpy(state,seed,SEED_SIZE*sizeof(octet));
  #endif
  next(); 
  //cout << "SetSeed : "; print_state(); cout << endl;
}


void PRNG::print_state() const
{
  int i;
  for (i=0; i<SEED_SIZE; i++)
    { if (seed[i]<10){ cout << "0"; }
      cout << hex << (int) seed[i]; 
    }
  cout << "\t";
  for (i=0; i<RAND_SIZE; i++)
    { if (random[i]<10) { cout << "0"; }
      cout << hex << (int) random[i]; 
    }
  cout << "\t";
  for (i=0; i<SEED_SIZE; i++)
    { if (state[i]<10) { cout << "0"; }
      cout << hex << (int) state[i];
    }
  cout << " " << dec << cnt << " : ";
}


void PRNG::hash()
{
  #ifndef USE_AES
    // Hash seed to get a random value
    blk_SHA_CTX ctx;
    blk_SHA1_Init(&ctx);
    blk_SHA1_Update(&ctx,state,SEED_SIZE);
    blk_SHA1_Final(random,&ctx);
  #else
    if (useC)
       { aes_encrypt(random,state,KeyScheduleC); }
    else
       { ecb_aes_128_encrypt<PIPELINES>((__m128i*)random,(__m128i*)state,KeySchedule); }
  #endif
  // This is a new random value so we have not used any of it yet
  cnt=0;
}



void PRNG::next()
{
  // Increment state
  for (int i = 0; i < PIPELINES; i++)
    {
      int64_t* s = (int64_t*)&state[i*AES_BLK_SIZE];
      s[0] += PIPELINES;
      if (s[0] == 0)
          s[1]++;
    }
  hash();
}


double PRNG::get_double()
{
  // We need four bytes of randomness
  if (cnt>RAND_SIZE-4) { next(); }
  unsigned int a0=random[cnt],a1=random[cnt+1],a2=random[cnt+2],a3=random[cnt+3];
  double ans=(a0+(a1<<8)+(a2<<16)+(a3<<24));
  cnt=cnt+4;
  unsigned int den=0xFFFFFFFF;
  ans=ans/den;
  //print_state(); cout << " DBLE " <<  ans << endl;
  return ans;
}


unsigned int PRNG::get_uint()
{
  // We need four bytes of randomness
  if (cnt>RAND_SIZE-4) { next(); }
  unsigned int a0=random[cnt],a1=random[cnt+1],a2=random[cnt+2],a3=random[cnt+3];
  cnt=cnt+4;
  unsigned int ans=(a0+(a1<<8)+(a2<<16)+(a3<<24));
  // print_state(); cout << " UINT " << ans << endl;
  return ans;
}



unsigned char PRNG::get_uchar()
{
  if (cnt>=RAND_SIZE) { next(); }
  unsigned char ans=random[cnt];
  cnt++;
  // print_state(); cout << " UCHA " << (int) ans << endl;
  return ans;
}


__m128i PRNG::get_doubleword()
{
    if (cnt > RAND_SIZE - 16)
        next();
    __m128i ans = _mm_loadu_si128((__m128i*)&random[cnt]);
    cnt += 16;
    return ans;
}


void PRNG::get_octetStream(octetStream& ans,int len)
{
  ans.resize(len);
  for (int i=0; i<len; i++)
    { ans.data[i]=get_uchar(); }
  ans.len=len;
  ans.ptr=0;
}


void PRNG::get_octets(octet* ans,int len)
{
  int pos=0;
  while (len)
    {
      int step=min(len,RAND_SIZE-cnt);
      memcpy(ans+pos,random+cnt,step);
      pos+=step;
      len-=step;
      cnt+=step;
      if (cnt==RAND_SIZE)
        next();
    }
}


bigint PRNG::randomBnd(const bigint& B, bool positive)
{
  bigint x;
#ifdef REALLOC_POLICE
  x = B;
#endif
  randomBnd(x, B, positive);
  return x;
}

void PRNG::randomBnd(bigint& x, const bigint& B, bool positive)
{
  int i = 0;
  do
    {
      get_bigint(x, numBits(B), true);
      if (i++ > 1000)
        {
          cout << x << " - " << B << " = " << x - B << endl;
          throw runtime_error("bounded randomness error");
        }
    }
  while (x >= B);
  if (!positive)
    {
      if (get_bit())
        mpz_neg(x.get_mpz_t(), x.get_mpz_t());
    }
}

void PRNG::get_bigint(bigint& res, int n_bits, bool positive)
{
  int n_bytes = (n_bits + 7) / 8;
  if (n_bytes > 1000)
    throw not_implemented();
  octet bytes[1000];
  get_octets(bytes, n_bytes);
  octet mask = (1 << (n_bits % 8)) - 1;
  bytes[0] &= mask;
  bigintFromBytes(res, bytes, n_bytes);
  if (not positive and (get_bit()))
    mpz_neg(res.get_mpz_t(), res.get_mpz_t());
}

void PRNG::get(bigint& res, int n_bits, bool positive)
{
  get_bigint(res, n_bits, positive);
}

void PRNG::get(int& res, int n_bits, bool positive)
{
  res = get_uint();
  res &= (1 << n_bits) - 1;
  if (positive and get_bit())
    res = -res;
}
