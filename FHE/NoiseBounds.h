// (C) 2018 University of Bristol. See License.txt

/*
 * NoiseBound.h
 *
 */

#ifndef FHE_NOISEBOUNDS_H_
#define FHE_NOISEBOUNDS_H_

#include "Math/bigint.h"

int phi_N(int N);

class SemiHomomorphicNoiseBounds
{
protected:
    const bigint p;
    const int phi_m;
    const int n;
    const int sec;
    int slack;
    const double sigma;
    const int h;

    bigint B_clean;
    bigint B_scale;
    bigint drown;

public:
    SemiHomomorphicNoiseBounds(const bigint& p, int phi_m, int n, int sec,
            int slack, bool extra_h = false, double sigma = 3.2, int h = 64);
    // with scaling
    bigint min_p0(const bigint& p1);
    // without scaling
    bigint min_p0();
    bigint min_p0(bool scale, const bigint& p1) { return scale ? min_p0(p1) : min_p0(); }
    static double min_phi_m(int log_q);
};

// as per ePrint 2012:642 for slack = 0
class NoiseBounds : public SemiHomomorphicNoiseBounds
{
    bigint B_KS;

public:
    NoiseBounds(const bigint& p, int phi_m, int n, int sec, int slack,
            double sigma = 3.2, int h = 64);
    bigint U1(const bigint& p0, const bigint& p1);
    bigint U2(const bigint& p0, const bigint& p1);
    bigint min_p0(const bigint& p0, const bigint& p1);
    bigint min_p0(const bigint& p1);
    bigint min_p1();
    bigint opt_p1();
    bigint opt_p0() { return min_p0(opt_p1()); }
    double optimize(int& lg2p0, int& lg2p1);
};

#endif /* FHE_NOISEBOUNDS_H_ */
