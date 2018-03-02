// (C) 2018 University of Bristol. See License.txt


#include "bigint.h"
#include "gfp.h"
#include "Exceptions/Exceptions.h"


bigint sqrRootMod(const bigint& a,const bigint& p)
{
  bigint ans;
  if (a==0) { ans=0; return ans; }
  if (mpz_legendre(a.get_mpz_t(), p.get_mpz_t()) != 1)
      throw runtime_error("cannot compute square root of non-square");
  if (mpz_tstbit(p.get_mpz_t(),1)==1)
    { // First do case with p=3 mod 4
      bigint exp=(p+1)/4;
      mpz_powm(ans.get_mpz_t(),a.get_mpz_t(),exp.get_mpz_t(),p.get_mpz_t());
    }
  else
    { // Shanks algorithm
      gmp_randclass Gen(gmp_randinit_default);
      Gen.seed(0);
      bigint x,y,n,q,t,b,temp;
      // Find n such that (n/p)=-1
      int leg=1;
      while (leg!=-1)
	  { n=Gen.get_z_range(p);
            leg=mpz_legendre(n.get_mpz_t(),p.get_mpz_t());
          }
      // Split p-1 = 2^e q
      q=p-1;
      int e=0;
      while (mpz_even_p(q.get_mpz_t())) 
        { e++; q=q/2; }
      // y=n^q mod p, x=a^((q-1)/2) mod p, r=e
      int r=e;
      mpz_powm(y.get_mpz_t(),n.get_mpz_t(),q.get_mpz_t(),p.get_mpz_t());
      temp=(q-1)/2;
      mpz_powm(x.get_mpz_t(),a.get_mpz_t(),temp.get_mpz_t(),p.get_mpz_t());
      // b=a*x^2 mod p, x=a*x mod p
      b=(a*x*x)%p;
      x=(a*x)%p;
      // While b!=1 do
      while (b!=1)
        { // Find smallest m such that b^(2^m)=1 mod p
          int m=1;
          temp=(b*b)%p;
          while (temp!=1)
           { temp=(temp*temp)%p; m++; }
          // t=y^(2^(r-m-1)) mod p, y=t^2, r=m
          t=y;
          for (int i=0; i<r-m-1; i++)
            { t=(t*t)%p; }
          y=(t*t)%p;
          r=m;
          // x=x*t mod p, b=b*y mod p
          x=(x*t)%p;
          b=(b*y)%p;
        }
      ans=x;
    }
  return ans;
}



bigint powerMod(const bigint& x,const bigint& e,const bigint& p)
{
  bigint ans;
  if (e>=0)
    { mpz_powm(ans.get_mpz_t(),x.get_mpz_t(),e.get_mpz_t(),p.get_mpz_t()); }
  else
    { bigint xi,ei=-e;
      invMod(xi,x,p);
      mpz_powm(ans.get_mpz_t(),xi.get_mpz_t(),ei.get_mpz_t(),p.get_mpz_t()); 
    }
      
  return ans;
}


int powerMod(int x,int e,int p)
{
  if (e==1) { return x; }
  if (e==0) { return 1; }
  if (e<0)
     { throw not_implemented(); }
   int t=x,ans=1;
   while (e!=0)
     { if ((e&1)==1) { ans=(ans*t)%p; }
       e>>=1;
       t=(t*t)%p;
     }
  return ans;
}


bigint::bigint(const gfp& x)
{
  to_bigint(*this, x);
}


size_t bigint::report_size(ReportType type) const
{
  size_t res = 0;
  if (type != MINIMAL)
    res += sizeof(*this);
  if (type == CAPACITY)
    res += get_mpz_t()->_mp_alloc * sizeof(mp_limb_t);
  else if (type == USED)
    res += abs(get_mpz_t()->_mp_size) * sizeof(mp_limb_t);
  else if (type == MINIMAL)
    res += 5 + numBytes(*this);
  return res;
}

template <>
int limb_size<bigint>()
{
  return 64;
}

template <>
int limb_size<int>()
{
  // doesn't matter
  return 0;
}

#ifdef REALLOC_POLICE
void bigint::lottery()
{
  if (rand() % 1000 == 0)
    if (rand() % 1000 == 0)
      throw runtime_error("much deallocation");
}
#endif
