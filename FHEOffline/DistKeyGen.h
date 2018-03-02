// (C) 2018 University of Bristol. See License.txt

/*
 * DistKeyGen.h
 *
 */

#ifndef FHEOFFLINE_DISTKEYGEN_H_
#define FHEOFFLINE_DISTKEYGEN_H_

#include "FHE/Random_Coins.h"
#include "FHE/Rq_Element.h"
#include "FHE/FHE_Params.h"
#include "FHE/FHE_Keys.h"
#include "Networking/Player.h"

void Encrypt_Rq_Element(Ciphertext& c, const Rq_Element& mess,
        const Random_Coins& rc, const FHE_PK& pk);

void Run_Gen_Protocol(FHE_PK& pk, FHE_SK& sk, const Player& P, int num_runs,
        bool commit);


class DistKeyGen
{
    const FHE_Params& params;

public:
    Random_Coins rc1, rc2;
    Rq_Element secret;
    Rq_Element a;
    Rq_Element e;
    Rq_Element b;
    FHE_PK pk;
    Ciphertext enc_dash;
    Ciphertext enc;
    bigint p;

    static void fake(FHE_PK& pk, vector<FHE_SK>& sks, const bigint& p, int nparties);

    DistKeyGen(const FHE_Params& params, const bigint& p);
    void Gen_Random_Data(PRNG& G);
    DistKeyGen& operator+=(const DistKeyGen& other);
    void sum_a(const vector<Rq_Element>& as);
    void compute_b();
    void compute_enc_dash(const vector<Rq_Element>& bs);
    void compute_enc_dash();
    void compute_enc(const vector<Ciphertext>& enc_dashs);
    void compute_enc();
    void sum_enc(const vector<Ciphertext>& enc);
    void finalize(FHE_PK& pk, FHE_SK& sk);
    void check_equality(const DistKeyGen other);
};

#endif /* FHEOFFLINE_DISTKEYGEN_H_ */
