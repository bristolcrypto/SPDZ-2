// (C) 2017 University of Bristol. See License.txt

/*
 * Setup.h
 *
 */

#ifndef MATH_SETUP_H_
#define MATH_SETUP_H_

#include "Math/bigint.h"

#include <iostream>
using namespace std;

/*
 * Routines to create and read setup files for the finite fields
 */

// Create setup file for gfp and gf2n
void generate_online_setup(ofstream& outf, string dirname, bigint& p, int lgp, int lg2);

// Setup primes only
// Chooses a p of at least lgp bits
void SPDZ_Data_Setup_Primes(bigint& p,int lgp,int& idx,int& m);

// get main directory for prep. data
string get_prep_dir(int nparties, int lg2p, int gf2ndegree);

// Read online setup file for gfp and gf2n
void read_setup(const string& dir_prefix);
void read_setup(int nparties, int lg2p, int gf2ndegree);


#endif /* MATH_SETUP_H_ */
