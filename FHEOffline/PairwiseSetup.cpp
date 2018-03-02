// (C) 2018 University of Bristol. See License.txt

/*
 * PairwiseSetup.cpp
 *
 */

#include <FHEOffline/PairwiseSetup.h>
#include "FHE/NoiseBounds.h"
#include "FHE/NTL-Subs.h"
#include "Math/Setup.h"
#include "FHEOffline/Proof.h"

template <class FD>
void PairwiseSetup<FD>::init(const Player& P, int sec, int plaintext_length,
    int& extra_slack)
{
    sec = max(sec, 40);
    cout << "Finding parameters for security " << sec << " and field size ~2^"
            << plaintext_length << endl;
    PRNG G;
    G.ReSeed();
    dirname = PREP_DIR;

    octetStream o;
    if (P.my_num() == 0)
    {
        extra_slack =
            generate_semi_setup(plaintext_length, sec, params, FieldD, true);
        params.pack(o);
        FieldD.pack(o);
        o.store(extra_slack);
        P.send_all(o);
    }
    else
    {
        P.receive_player(0, o);
        params.unpack(o);
        FieldD.unpack(o);
        FieldD.init_field();
        o.get(extra_slack);
    }

    alpha = FieldD;
    alpha.randomize(G, Diagonal);
    alphai = alpha.element(0);
}

template class PairwiseSetup<FFT_Data>;
template class PairwiseSetup<P2Data>;
