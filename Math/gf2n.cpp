// (C) 2018 University of Bristol. See License.txt


#include "Math/gf2n.h"

#include "Exceptions/Exceptions.h"

#include <stdint.h>
#include <wmmintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>

int gf2n_short::n;
int gf2n_short::t1;
int gf2n_short::t2;
int gf2n_short::t3;
int gf2n_short::l0;
int gf2n_short::l1;
int gf2n_short::l2;
int gf2n_short::l3;
int gf2n_short::nterms;
word gf2n_short::mask;
bool gf2n_short::useC;
bool gf2n_short::rewind = false;

word gf2n_short_table[256][256];

#define num_2_fields 4

/* Require
 *  2*(n-1)-64+t1<64
 */
int fields_2[num_2_fields][4] = { 
	{4,1,0,0},{8,4,3,1},{28,1,0,0},{40,20,15,10}
    };


void gf2n_short::init_tables()
{
  if (sizeof(word)!=8)
    { cout << "Word size is wrong" << endl; 
      throw not_implemented();
    }
  int i,j;
  for (i=0; i<256; i++)
    { for (j=0; j<256; j++)
        { word ii=i,jj=j;
          gf2n_short_table[i][j]=0;
          while (ii!=0)
            { if ((ii&1)==1) { gf2n_short_table[i][j]^=jj; }
              jj<<=1;
              ii>>=1;
            }
         }
    }
}


void gf2n_short::init_field(int nn)
{
  if (nn == 0)
    {
      nn = default_length();
      cerr << "Using GF(2^" << nn << ")" << endl;
    }

  gf2n_short::init_tables();
  int i,j=-1;
  for (i=0; i<num_2_fields && j==-1; i++)
    { if (nn==fields_2[i][0]) { j=i; } }
  if (j==-1)
    {
      if (nn == 128)
	throw runtime_error("need to compile with USE_GF2N_LONG = 1; "
			    "remember to make clean");
      else
	throw runtime_error("field size not supported");
    }

  n=nn;
  nterms=1;
  l0=64-n;  
  t1=fields_2[j][1];
  l1=64+t1-n; 
  if (fields_2[j][2]!=0)
    { nterms=3;
      t2=fields_2[j][2];
      l2=64+t2-n; 
      t3=fields_2[j][3];
      l3=64+t3-n; 
    }
  if (2*(n-1)-64+t1>=64) { throw invalid_params(); }

  mask=(1ULL<<n)-1;

  useC=(Check_CPU_support_AES()==0);
}
  


/* Takes 16bit x and y and returns the 32 bit product in c1 and c0
      ans = (c1<<16)^c0
   where c1 and c0 are 16 bit
*/
inline void mul16(word x,word y,word& c0,word& c1)
{
  word a1=x&(0xFF), b1=y&(0xFF);
  word a2=x>>8,     b2=y>>8;

  c0=gf2n_short_table[a1][b1];
  c1=gf2n_short_table[a2][b2];
  word te=gf2n_short_table[a1][b2]^gf2n_short_table[a2][b1];
  c0^=(te&0xFF)<<8;
  c1^=te>>8;
}

/* Takes 16 bit x and y and returns the 32 bit product */
inline word mul16(word x,word y)
{
  word a1=x&(0xFF), b1=y&(0xFF);
  word a2=x>>8,     b2=y>>8;

  word ans=gf2n_short_table[a2][b2]<<8;
  ans^=gf2n_short_table[a1][b2]^gf2n_short_table[a2][b1];
  ans<<=8;
  ans^=gf2n_short_table[a1][b1];

  return ans;
}



/* Takes 16 bit x the 32 bit square */
inline word sqr16(word x)
{
  word a1=x&(0xFF),a2=x>>8;

  word ans=gf2n_short_table[a2][a2]<<16;
  ans^=gf2n_short_table[a1][a1];

  return ans;
}



void gf2n_short::reduce_trinomial(word xh,word xl)
{
  // Deal with xh first
  a=xl;
  a^=(xh<<l0); 
  a^=(xh<<l1); 

  // Now deal with last word
  word hi=a>>n;
  while (hi!=0)
    { a&=mask;

      a^=hi;
      a^=(hi<<t1);
      hi=a>>n;
    }
}

void gf2n_short::reduce_pentanomial(word xh,word xl)
{
  // Deal with xh first
  a=xl;
  a^=(xh<<l0);   
  a^=(xh<<l1);
  a^=(xh<<l2);
  a^=(xh<<l3);

  // Now deal with last word
  word hi=a>>n;
  while (hi!=0)
    { a&=mask;

      a^=hi;
      a^=(hi<<t1);
      a^=(hi<<t2);
      a^=(hi<<t3);

      hi=a>>n;
    }
}


void mul32(word x,word y,word& ans)
{
  word a1=x&(0xFFFF),b1=y&(0xFFFF);
  word a2=x>>16,     b2=y>>16;

  word c0,c1;

  ans=mul16(a1,b1);
  word upp=mul16(a2,b2);

  mul16(a1,b2,c0,c1);
  ans^=c0<<16;       upp^=c1;

  mul16(a2,b1,c0,c1);
  ans^=c0<<16;       upp^=c1;

  ans^=(upp<<32);
}



void gf2n_short::mul(const gf2n_short& x,const gf2n_short& y)
{
  word hi,lo;
  
  if (gf2n_short::useC)
    { /* Uses Karatsuba */
      word c,d,e,t;
      word xl=x.a&0xFFFFFFFF,yl=y.a&0xFFFFFFFF;
      word xh=x.a>>32,yh=y.a>>32;
      mul32(xl,yl,c);
      mul32(xh,yh,d);
      mul32((xl^xh),(yl^yh),e);
      t=c^e^d;
      lo=c^(t<<32);
      hi=d^(t>>32);
    }
  else
    { /* Use Intel Instructions */
      __m128i xx,yy,zz;
      uint64_t c[] __attribute__((aligned (16))) = { 0,0 };
      xx=_mm_set1_epi64x(x.a);
      yy=_mm_set1_epi64x(y.a);
      zz=_mm_clmulepi64_si128(xx,yy,0);
      _mm_store_si128((__m128i*)c,zz);
      lo=c[0];
      hi=c[1];
    }

  reduce(hi,lo);
}




inline void sqr32(word x,word& ans)
{
  word a1=x&(0xFFFF),a2=x>>16;
  ans=sqr16(a1)^(sqr16(a2)<<32);
}

void gf2n_short::square()
{
  word xh,xl;
  sqr32(a&0xFFFFFFFF,xl);
  sqr32(a>>32,xh);
  reduce(xh,xl);
}


void gf2n_short::square(const gf2n_short& bb)
{
  word xh,xl;
  sqr32(bb.a&0xFFFFFFFF,xl);
  sqr32(bb.a>>32,xh);
  reduce(xh,xl);
}






void gf2n_short::invert()
{
  if (is_one())  { return; }
  if (is_zero()) { throw division_by_zero(); }

  word u,v=a,B=0,D=1,mod=1;

  mod^=(1ULL<<n);
  mod^=(1ULL<<t1);
  if (nterms==3)
    { mod^=(1ULL<<t2);
      mod^=(1ULL<<t3);
    }
  u=mod; v=a;
  
  while (u!=0)
    { while ((u&1)==0)
	    { u>>=1;
          if ((B&1)!=0) { B^=mod; }
 	      B>>=1;
        }
      while ((v&1)==0 && v!=0)
	    { v>>=1;
	      if ((D&1)!=0) { D^=mod; }
		  D>>=1;
	    }

      if (u>=v) { u=u^v; B=B^D; }
	  else      { v=v^u; D=D^B; }
   }

  a=D;
}


void gf2n_short::power(long i)
{ 
  long n=i;
  if (n<0) { invert(); n=-n; }

  gf2n_short T=*this;
  assign_one();
  while (n!=0)
    { if ((n&1)!=0) { mul(*this,T); }
       n>>=1;
       T.square();
    }
}


void gf2n_short::randomize(PRNG& G)
{
  a=G.get_uint();
  a=(a<<32)^G.get_uint();
  a&=mask;
}


void gf2n_short::output(ostream& s,bool human) const
{
  if (human)
    { s << hex << showbase << a << dec << " "; }
  else
    { s.write((char*) &a,sizeof(word)); }
}

void gf2n_short::input(istream& s,bool human)
{
  if (s.peek() == EOF)
    { if (s.tellg() == 0)
        { cout << "IO problem. Empty file?" << endl;
          throw file_error();
        }
      throw end_of_file("gf2n_short");
    }

  if (human)
    { s >> hex >> a >> dec; } 
  else
    { s.read((char*) &a,sizeof(word)); }

  a &= mask;
}
