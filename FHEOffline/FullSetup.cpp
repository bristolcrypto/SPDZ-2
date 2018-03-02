// (C) 2018 University of Bristol. See License.txt


#include "FHEOffline/FullSetup.h"
#include "Math/gfp.h"

#include "FHE/FHE_Params.h"
#include "FHE/NTL-Subs.h"

#include <sys/stat.h>
#include <fstream>
#include <sstream>
using namespace std;


// Read data for SHE offline
void get_setup(FHE_Params& params_p,FFT_Data& FTD,
               FHE_Params& params_2,P2Data& P2D,string dir,
               bool skip_2)
{
  int lg2;
  Ring R2,Rp;
  bigint p02,p12,p0p,p1p,p;

  // legacy
  if (dir == "")
    dir = PREP_DIR;

  string filename=dir+"/Params-Data";
  
  ifstream inpf(filename.c_str());
  if (inpf.fail()) { throw file_error(filename); }
  inpf >> p;
  inpf >> lg2;
  inpf >> Rp;
  inpf >> FTD;
  inpf >> p0p >> p1p;

  if (p != FTD.get_prime())
    throw runtime_error("inconsistent p in Params-Data");

  if (!skip_2)
    {
      // initialize before reading P2D for consistency check
      gf2n::init_field(lg2);
      inpf >> R2;
      inpf >> P2D;
      if (R2.phi_m() != P2D.phi_m())
        throw runtime_error("phi(m) mismatch between ring and plaintext representation");
      inpf >> p02 >> p12;
    }

  if (inpf.fail())
      throw file_error("incomplete parameters");

  inpf.close();

  gfp::init_field(FTD.get_prime());
  params_p.set(Rp,{p0p,p1p});
  cout << "log(p) = " << numBits(FTD.get_prime()) << ", log(p0) = "
      << numBits(p0p) << ", " << "log(p1) = " << numBits(p1p) << endl;

  if (!skip_2)
    {
      params_2.set(R2,{p02,p12});
      cout << "GF(2^" << lg2 << "): log(p0) = " << numBits(p02)
          << ", log(p1) = " << numBits(p12) << endl;
    }
}
