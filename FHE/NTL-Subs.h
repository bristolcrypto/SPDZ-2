// (C) 2018 University of Bristol. See License.txt

#ifndef _NTL_Subs
#define _NTL_Subs

/* All these routines use NTL on the inside */

#include "FHE/Ring.h"
#include "FHE/FFT_Data.h"
#include "FHE/P2Data.h"
#include "FHE/PPData.h"
#include "FHE/FHE_Params.h"


/* Routines to set up key sizes given the number of players n
 *  - And size lgp of plaintext modulus p for the char p case
 */

// Main setup routine (need NTL if online_only is false)
void generate_setup(int nparties, int lgp, int lg2,
    int sec, bool skip_2 = false, int slack = 0, bool round_up = false);

template <class FD>
void generate_setup(int n_parties, int plaintext_length, int sec,
    FHE_Params& params, FD& FTD, int slack, bool round_up);

// semi-homomorphic, includes slack
template <class FD>
int generate_semi_setup(int plaintext_length, int sec,
    FHE_Params& params, FD& FieldD, bool round_up);

// field-independent semi-homomorphic setup
int common_semi_setup(FHE_Params& params, int m, bigint p, int lgp0, int lgp1,
    bool round_up);

// Everything else needs NTL
void init(Ring& Rg,int m);
void init(P2Data& P2D,const Ring& Rg);

// For use when we only care about p being of a certain size
void SPDZ_Data_Setup_Char_p(Ring& R, FFT_Data& FTD, bigint& pr0, bigint& pr1,
    int n, int lgp, int sec, int slack = 0, bool round_up = false);

// For use when we want p to be a specific value 
void SPDZ_Data_Setup_Char_p_General(Ring& R, PPData& PPD, bigint& pr0,
    bigint& pr1, int n, int sec, bigint& p);

// generate moduli according to lengths and other parameters
void generate_moduli(bigint& pr0, bigint& pr1, const int m,
        const bigint p, const int lg2p0, const int lg2p1);

void generate_modulus(bigint& pr, const int m, const bigint p, const int lg2pr,
    const string& i = "0", const bigint& pr0 = 0);

// pre-generated dimensions for characteristic 2
void char_2_dimension(int& m, int& lg2);

void SPDZ_Data_Setup_Char_2(Ring& R, P2Data& P2D, bigint& pr0, bigint& pr1,
    int n, int lg2, int sec = -1, int slacke = 0, bool round_up = false);

// try to avoid expensive generation by loading from disk if possible
void load_or_generate(P2Data& P2D, const Ring& Rg);

int phi_N(int N);

#endif
