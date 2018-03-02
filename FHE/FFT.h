// (C) 2018 University of Bristol. See License.txt

#ifndef _FFT
#define _FFT

/* FFT routine (mod p) for when N is a power of 2
     - We do not check that N is a power of 2 though
   This is the "classic" FFT algorithm
   theta is assumed to be an Nth root of unity

   Not tried to optimize memory thrashing though
*/

#include <vector>

#include "FHE/FFT_Data.h"

/* This is the basic 2^n - FFT routine, it works in an recursive 
 * manner. The FFT_Iter routine below does the same thing but in 
 * an iterative manner, and is therefore more efficient.
 */
void FFT(vector<modp>& a,int N,const modp& theta,const Zp_Data& PrD);

/* This uses a different root of unity and is used for going
 * forward when doing 2^n FFT's. This avoids the need for padding
 * out to a vector of size 2*n. Going backwards you still need to
 * use FFT, but some other post-processing. Again the iterative
 * version is given by FFT_Iter2 below, and this is usually faster
 */
void FFT2(vector<modp>& a,int N,const modp& theta,const Zp_Data& PrD);

template <class T,class P>
void FFT_Iter(vector<T>& a,int N,const T& theta,const P& PrD);

void FFT_Iter2(vector<modp>& a,int N,const modp& theta,const Zp_Data& PrD);


/* BFFT perform FFT and inverse FFT  mod PrD for non power of two cyclotomics.  
 * The modulus in PrD (contained in FFT_Data) must be set up
 * correctly in particular
 *     m divides pr-1     <- Otherwise FFT not defined over base field
 *     2^k divides pr-1   <- Where 2^k is the smallest power of two
 *                           greater than m. This means we can do the
 *                           trick of extension and the convolution using
 *                           the 2-power FFT to do the main work
 * forward=true  => FFT
 * forward=false => inverse FFT
 *
 * a on input is assumed to have size less than FFTD.n (it is then zero padded)
 * ans is assumed to have size FFTD.n
 */                           

void BFFT(vector<modp>& ans,const vector<modp>& a,const FFT_Data& FFTD,bool forward=true);


/* Computes the FFT via Horner's Rule
   theta is assumed to be an Nth root of unity
   This works for any value of N
*/
void NaiveFFT(vector<modp>& ans,vector<modp>& a,int N,const modp& theta,const Zp_Data& PrD);


#endif

