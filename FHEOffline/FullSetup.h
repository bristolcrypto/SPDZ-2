// (C) 2018 University of Bristol. See License.txt

#ifndef _FullSetup
#define _FullSetup

/* Reads in the data created by the setup program */

#include "FHE/FHE_Params.h"
#include "FHE/P2Data.h"
#include "Math/Setup.h"

void get_setup(FHE_Params& params_p,FFT_Data& FTD,
               FHE_Params& params_2,P2Data& P2D,string dir,
               bool skip_2=false);

#endif


