// (C) 2018 University of Bristol. See License.txt

/*
 * SimpleDistDecrypt.cpp
 *
 */

#include <FHEOffline/SimpleDistDecrypt.h>

template <class FD>
void SimpleDistDecrypt<FD>::intermediate_step()
{
    for (unsigned int i = 0; i < this->vv.size(); i++)
        this->vv[i] += this->f.coeff(i);
}

template <class FD>
void SimpleDistDecrypt<FD>::reshare(Plaintext<typename FD::T, FD, typename FD::S>& m,
        const Ciphertext& cm,
        EncCommitBase<typename FD::T, FD, typename FD::S>& EC)
{
    (void)EC;

    PRNG G;
    G.ReSeed();
    this->f.randomize(G, Full);

    // Step 3
    this->run(cm);

    // Step 4
    if (this->P.my_num()==0)
      { sub(m,this->mf,this->f); }
    else
      { m=this->f; m.negate(); }
}


template class SimpleDistDecrypt<FFT_Data>;
template class SimpleDistDecrypt<P2Data>;
