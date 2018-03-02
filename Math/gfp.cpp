// (C) 2018 University of Bristol. See License.txt


#include "Math/gfp.h"

#include "Exceptions/Exceptions.h"

Zp_Data gfp::ZpD;

void gfp::almost_randomize(PRNG& G)
{
  G.get_octets((octet*)a.x,t()*sizeof(mp_limb_t));
  a.x[t()-1]&=ZpD.mask;
}

void gfp::AND(const gfp& x,const gfp& y)
{
  bigint bi1,bi2;
  to_bigint(bi1,x);
  to_bigint(bi2,y);
  mpz_and(bi1.get_mpz_t(), bi1.get_mpz_t(), bi2.get_mpz_t());
  to_gfp(*this, bi1);
}

void gfp::OR(const gfp& x,const gfp& y)
{
  bigint bi1,bi2;
  to_bigint(bi1,x);
  to_bigint(bi2,y);
  mpz_ior(bi1.get_mpz_t(), bi1.get_mpz_t(), bi2.get_mpz_t());
  to_gfp(*this, bi1);
}

void gfp::XOR(const gfp& x,const gfp& y)
{
  bigint bi1,bi2;
  to_bigint(bi1,x);
  to_bigint(bi2,y);
  mpz_xor(bi1.get_mpz_t(), bi1.get_mpz_t(), bi2.get_mpz_t());
  to_gfp(*this, bi1);
}

void gfp::AND(const gfp& x,const bigint& y)
{
  bigint bi;
  to_bigint(bi,x);
  mpz_and(bi.get_mpz_t(), bi.get_mpz_t(), y.get_mpz_t());
  to_gfp(*this, bi);
}

void gfp::OR(const gfp& x,const bigint& y)
{
  bigint bi;
  to_bigint(bi,x);
  mpz_ior(bi.get_mpz_t(), bi.get_mpz_t(), y.get_mpz_t());
  to_gfp(*this, bi);
}

void gfp::XOR(const gfp& x,const bigint& y)
{
  bigint bi;
  to_bigint(bi,x);
  mpz_xor(bi.get_mpz_t(), bi.get_mpz_t(), y.get_mpz_t());
  to_gfp(*this, bi);
}




void gfp::SHL(const gfp& x,int n)
{
  if (!x.is_zero())
    {
      if (n != 0)
        {
          bigint bi;
          to_bigint(bi,x,false);
          mpn_lshift(bi.get_mpz_t()->_mp_d, bi.get_mpz_t()->_mp_d, bi.get_mpz_t()->_mp_size,n);
          to_gfp(*this, bi);
        }
      else
        assign(x);
    }
  else
    {
      assign_zero();
    }
}


void gfp::SHR(const gfp& x,int n)
{
  if (!x.is_zero())
    {
      if (n != 0)
        {
          bigint bi;
          to_bigint(bi,x);
          mpn_rshift(bi.get_mpz_t()->_mp_d, bi.get_mpz_t()->_mp_d, bi.get_mpz_t()->_mp_size,n);
          to_gfp(*this, bi);
        }
      else
        assign(x);
    }
  else
    {
      assign_zero();
    }
}


void gfp::SHL(const gfp& x,const bigint& n)
{
  SHL(x,mpz_get_si(n.get_mpz_t()));
}


void gfp::SHR(const gfp& x,const bigint& n)
{
  SHR(x,mpz_get_si(n.get_mpz_t()));
}


gfp gfp::sqrRoot()
{
    // Temp move to bigint so as to call sqrRootMod
    bigint ti;
    to_bigint(ti, *this);
    ti = sqrRootMod(ti, ZpD.pr);
    if (!isOdd(ti))
        ti = ZpD.pr - ti;
    gfp temp;
    to_gfp(temp, ti);
    return temp;
}
