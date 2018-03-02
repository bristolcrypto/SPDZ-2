// (C) 2018 University of Bristol. See License.txt

/*
 * NoiseBound.cpp
 *
 */

#include <FHE/NoiseBounds.h>
#include "FHEOffline/Proof.h"
#include <math.h>


SemiHomomorphicNoiseBounds::SemiHomomorphicNoiseBounds(const bigint& p,
        int phi_m, int n, int sec, int slack_param, bool extra_h, double sigma, int h) :
        p(p), phi_m(phi_m), n(n), sec(sec),
        slack(numBits(Proof::slack(slack_param, sec, phi_m))), sigma(sigma), h(h)
{
    h += extra_h * sec;
    B_clean = (phi_m * p / 2
            + p * sigma
                    * (16 * phi_m * sqrt(n / 2) + 6 * sqrt(phi_m)
                            + 16 * sqrt(n * h * phi_m))) << slack;
    B_scale = p * sqrt(3 * phi_m) * (1 + 8 * sqrt(n * h) / 3);
    drown = 1 + (bigint(1) << sec);
    cout << "log(slack): " << slack << endl;
}

bigint SemiHomomorphicNoiseBounds::min_p0(const bigint& p1)
{
    return p * drown * n * B_clean / p1 + B_scale;
}

bigint SemiHomomorphicNoiseBounds::min_p0()
{
    // slack is already in B_clean
    return B_clean * drown * p;
}

double SemiHomomorphicNoiseBounds::min_phi_m(int log_q)
{
    return 33.1 * (log_q - log2(3.2));
}


NoiseBounds::NoiseBounds(const bigint& p, int phi_m, int n, int sec, int slack,
        double sigma, int h) :
        SemiHomomorphicNoiseBounds(p, phi_m, n, sec, slack, false, sigma, h)
{
    B_KS = p * phi_m * sigma
            * (pow(n, 2.5) * (1.49 * sqrt(h * phi_m) + 2.11 * h)
                    + 2.77 * n * n * sqrt(h)
                    + pow(n, 1.5) * (1.96 * sqrt(phi_m) * 2.77 * sqrt(h))
                    + 4.62 * n);
#ifdef NOISY
    cout << "p size: " << numBits(p) << endl;
    cout << "phi(m): " << phi_m << endl;
    cout << "n: " << n << endl;
    cout << "sec: " << sec << endl;
    cout << "sigma: " << sigma << endl;
    cout << "h: " << h << endl;
    cout << "B_clean size: " << numBits(B_clean) << endl;
    cout << "B_scale size: " << numBits(B_scale) << endl;
    cout << "B_KS size: " << numBits(B_KS) << endl;
    cout << "drown size: " << numBits(drown) << endl;
#endif
}

bigint NoiseBounds::U1(const bigint& p0, const bigint& p1)
{
    bigint tmp = n * B_clean / p1 + B_scale;
    return tmp * tmp + B_KS * p0 / p1 + B_scale;
}

bigint NoiseBounds::U2(const bigint& p0, const bigint& p1)
{
    return U1(p0, p1) + n * B_clean / p1 + B_scale;
}

bigint NoiseBounds::min_p0(const bigint& p0, const bigint& p1)
{
    return 2 * U2(p0, p1) * drown;
}

bigint NoiseBounds::min_p0(const bigint& p1)
{
    bigint U = n * B_clean / p1 + 1 + B_scale;
    bigint res = 2 * (U * U + U + B_scale) * drown;
    mpf_class div = (1 - 1. * min_p1() / p1);
    res = ceil(res / div);
#ifdef NOISY
    cout << "U size: " << numBits(U) << endl;
    cout << "before div size: " << numBits(res) << endl;
    cout << "div: " << div << endl;
    cout << "minimal p0 size: " << numBits(res) << endl;
#endif
    return res;
}

bigint NoiseBounds::min_p1()
{
    return drown * B_KS + 1;
}

bigint NoiseBounds::opt_p1()
{
    // square equation parameters
    bigint a, b, c;
    a = B_scale * B_scale + B_scale;
    b = -2 * a * min_p1();
    c = -n * B_clean * (2 * B_scale + 1) * min_p1() + n * n * B_scale * B_scale;
    // solve
    mpf_class s = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
    bigint res = ceil(s);
    cout << "Optimal p1 vs minimal: " << numBits(res) << "/"
            << numBits(min_p1()) << endl;
    return res;
}

double NoiseBounds::optimize(int& lg2p0, int& lg2p1)
{
    bigint min_p1 = opt_p1();
    bigint min_p0 = this->min_p0(min_p1);
    while (this->min_p0(min_p0, min_p1) > min_p0)
      {
        min_p0 *= 2;
        min_p1 *= 2;
        cout << "increasing lengths: " << numBits(min_p0) << "/"
            << numBits(min_p1) << endl;
      }
    lg2p1 = numBits(min_p1);
    lg2p0 = numBits(min_p0);
    return min_phi_m(lg2p0 + lg2p1);
}
