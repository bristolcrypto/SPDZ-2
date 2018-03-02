// (C) 2018 University of Bristol. See License.txt


#include "FHE_Params.h"
#include "FHE/Ring_Element.h"
#include "Math/gfp.h"
#include "Exceptions/Exceptions.h"


void FHE_Params::set(const Ring& R,
                     const vector<bigint>& primes,double r,int hwt)
{
  if (primes.size() != FFTData.size())
    throw runtime_error("wrong number of primes");

  for (size_t i = 0; i < FFTData.size(); i++)
    FFTData[i].init(R,primes[i]);

  Chi.set(R.phi_m(),hwt,r);

  set_sec(40);
}

void FHE_Params::set_sec(int sec)
{
  sec_p=sec;
  Bval=1;  Bval=Bval<<sec_p;
  Bval=FFTData[0].get_prime()/(2*(1+Bval));
  if (Bval == 0)
    throw runtime_error("distributed decryption bound is zero");
}

bigint FHE_Params::Q() const
{
  bigint res = FFTData[0].get_prime();
  for (size_t i = 1; i < FFTData.size(); i++)
    res *= FFTData[i].get_prime();
  return res;
}

void FHE_Params::pack(octetStream& o) const
{
  o.store(FFTData.size());
  for(auto& fd: FFTData)
    fd.pack(o);
  Chi.pack(o);
  Bval.pack(o);
  o.store(sec_p);
}

void FHE_Params::unpack(octetStream& o)
{
  size_t size;
  o.get(size);
  FFTData.resize(size);
  for (auto& fd : FFTData)
    fd.unpack(o);
  Chi.unpack(o);
  Bval.unpack(o);
  o.get(sec_p);
}

bool FHE_Params::operator!=(const FHE_Params& other) const
{
  if (FFTData != other.FFTData or Chi != other.Chi or sec_p != other.sec_p
      or Bval != other.Bval)
    {
      return true;
    }
  else
    return false;
}
