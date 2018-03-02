// (C) 2018 University of Bristol. See License.txt

/*
 * PairwiseMachine.cpp
 *
 */

#include "FHEOffline/PairwiseMachine.h"
#include "Tools/benchmarking.h"
#include "Auth/fake-stuff.h"

PairwiseMachine::PairwiseMachine(int argc, const char** argv) :
    MachineBase(argc, argv), P(N, 0xffff << 16),
    other_pks(N.num_players(), {setup_p.params, 0}),
    pk(other_pks[N.my_num()]), sk(pk)
{
    if (use_gf2n)
    {
        field_size = 40;
        gf2n::init_field(field_size);
        setup_keys<P2Data>();
    }
    else
    {
        setup_keys<FFT_Data>();
        bigint p = setup_p.FieldD.get_prime();
        gfp::init_field(p);
        ofstream outf;
        if (output)
            write_online_setup(outf, PREP_DIR, p, 40);
    }

    for (int i = 0; i < nthreads; i++)
        if (use_gf2n)
            generators.push_back(new PairwiseGenerator<P2Data>(i, *this));
        else
            generators.push_back(new PairwiseGenerator<FFT_Data>(i, *this));
}

template <>
PairwiseSetup<FFT_Data>& PairwiseMachine::setup()
{
    return setup_p;
}

template <>
PairwiseSetup<P2Data>& PairwiseMachine::setup()
{
    return setup_2;
}

template <class FD>
void PairwiseMachine::setup_keys()
{
    PairwiseSetup<FD>& s = setup<FD>();
    s.init(P, sec, field_size, extra_slack);
    if (output)
        write_mac_keys(PREP_DIR, P.my_num(), P.num_players(), setup_p.alphai,
                setup_2.alphai);
    for (auto& x : other_pks)
        x = FHE_PK(s.params, s.FieldD.get_prime());
    sk = FHE_SK(pk);
    PRNG G;
    G.ReSeed();
    insecure("local key generation");
    KeyGen(pk, sk, G);
    vector<octetStream> os(N.num_players());
    pk.pack(os[N.my_num()]);
    P.Broadcast_Receive(os);
    for (int i = 0; i < N.num_players(); i++)
        if (i != N.my_num())
            other_pks[i].unpack(os[i]);

    insecure("MAC key generation");
    Ciphertext enc_alpha = pk.encrypt(s.alpha);
    os.clear();
    os.resize(N.num_players());
    enc_alphas.resize(N.num_players(), pk);
    enc_alpha.pack(os[N.my_num()]);
    P.Broadcast_Receive(os);
    for (int i = 0; i < N.num_players(); i++)
        if (i != N.my_num())
            enc_alphas[i].unpack(os[i]);
    for (int i = 0; i < N.num_players(); i++)
        cout << "Player " << i << " has pk "
                << other_pks[i].a().get(0).get_constant().get_limb(0) << " ..."
                << endl;
}

template void PairwiseMachine::setup_keys<FFT_Data>();
template void PairwiseMachine::setup_keys<P2Data>();
