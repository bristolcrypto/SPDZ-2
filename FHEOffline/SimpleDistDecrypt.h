// (C) 2018 University of Bristol. See License.txt

/*
 * SimpleDistDecrypt.h
 *
 */

#ifndef FHEOFFLINE_SIMPLEDISTDECRYPT_H_
#define FHEOFFLINE_SIMPLEDISTDECRYPT_H_

#include "FHEOffline/DistDecrypt.h"
#include "FHEOffline/DataSetup.h"

template <class FD>
class SimpleDistDecrypt : public DistDecrypt<FD>
{
public:
    SimpleDistDecrypt(const Player& P, const PartSetup<FD>& setup) :
            DistDecrypt<FD>(P, setup.sk, setup.pk, setup.FieldD) {}

    void intermediate_step();
    void reshare(Plaintext<typename FD::T, FD, typename FD::S>& m,
        const Ciphertext& cm,
        EncCommitBase<typename FD::T, FD, typename FD::S>& EC);
};

#endif /* FHEOFFLINE_SIMPLEDISTDECRYPT_H_ */
