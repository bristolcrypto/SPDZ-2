// (C) 2018 University of Bristol. See License.txt

/*
 * SimpleEncCommit.h
 *
 */

#ifndef FHEOFFLINE_SIMPLEENCCOMMIT_H_
#define FHEOFFLINE_SIMPLEENCCOMMIT_H_

#include "FHEOffline/EncCommit.h"
#include "FHEOffline/Proof.h"
#include "FHEOffline/Prover.h"
#include "FHEOffline/Verifier.h"
#include "Tools/MemoryUsage.h"

class MachineBase;

typedef map<string, Timer> TimerMap;

template<class T,class FD,class S>
class SimpleEncCommitBase : public EncCommitBase<T,FD,S>
{
protected:
    int sec;
    int extra_slack;

    int n_rounds;

    void generate_ciphertexts(AddableVector<Ciphertext>& c,
            const vector<Plaintext_<FD> >& m, Proof::Randomness& r,
            const FHE_PK& pk, map<string, Timer>& timers);

    virtual void prepare_plaintext(PRNG& G) = 0;

public:
    MemoryUsage memory_usage;

    SimpleEncCommitBase(const MachineBase& machine);
    virtual ~SimpleEncCommitBase() {}
    void report_size(ReportType type, MemoryUsage& res) { (void)type; res += memory_usage; }
};

template <class FD>
using SimpleEncCommitBase_ = SimpleEncCommitBase<typename FD::T, FD, typename FD::S>;

template <class FD>
class NonInteractiveProofSimpleEncCommit : public SimpleEncCommitBase_<FD>
{
protected:
    typedef bigint S;

    const PlayerBase& P;
    const FHE_PK& pk;
    const FD& FTD;

    NonInteractiveProof proof;

#ifdef LESS_ALLOC_MORE_MEM
    Proof::Randomness r;
    Prover<FD, Plaintext_<FD> > prover;
    Verifier<FD,S> verifier;
#endif

    virtual const FHE_PK& get_pk_for_verification(int offset) = 0;
    virtual void add_ciphertexts(vector<Ciphertext>& ciphertexts, int offset) = 0;

public:
    map<string, Timer>& timers;

    NonInteractiveProofSimpleEncCommit(const PlayerBase& P, const FHE_PK& pk,
            const FD& FTD, map<string, Timer>& timers,
				       const MachineBase& machine);
    virtual ~NonInteractiveProofSimpleEncCommit() {}
    size_t generate_proof(AddableVector<Ciphertext>& c, const vector<Plaintext_<FD> >& m,
            octetStream& ciphertexts, octetStream& cleartexts);
    size_t create_more(octetStream& my_ciphertext, octetStream& my_cleartext);
    virtual size_t report_size(ReportType type);
    using SimpleEncCommitBase_<FD>::report_size;
};

template <class FD>
class SimpleEncCommitFactory
{
protected:
    int cnt;
    AddableVector<Ciphertext> c;
    AddableVector< Plaintext_<FD> > m;

    int n_calls;

    void prepare_plaintext(PRNG& G);
    virtual void create_more() = 0;

public:
    SimpleEncCommitFactory(const FHE_PK& pk, const FD& FTD, const MachineBase& machine);
    virtual ~SimpleEncCommitFactory();
    bool has_left() { return cnt >= 0; }
    void next(Plaintext_<FD>& mess, Ciphertext& C);
    virtual size_t report_size(ReportType type);
    void report_size(ReportType type, MemoryUsage& res);
};

template<class T,class FD,class S>
class SimpleEncCommit: public NonInteractiveProofSimpleEncCommit<FD>,
        public SimpleEncCommitFactory<FD>
{
protected:
    const FHE_PK& get_pk_for_verification(int offset) { (void)offset; return this->pk; }
    void prepare_plaintext(PRNG& G)
    { SimpleEncCommitFactory<FD>::prepare_plaintext(G); }
    void add_ciphertexts(vector<Ciphertext>& ciphertexts, int offset);

public:
    SimpleEncCommit(const PlayerBase& P, const FHE_PK& pk, const FD& FTD,
            map<string, Timer>& timers, const MachineBase& machine, int thread_num);
    void next(Plaintext_<FD>& mess, Ciphertext& C) { SimpleEncCommitFactory<FD>::next(mess, C); }
    void create_more();
    size_t report_size(ReportType type)
    { return SimpleEncCommitFactory<FD>::report_size(type) + EncCommitBase_<FD>::report_size(type); }
    void report_size(ReportType type, MemoryUsage& res)
    { SimpleEncCommitFactory<FD>::report_size(type, res); SimpleEncCommitBase_<FD>::report_size(type, res); }
};

template <class FD>
using SimpleEncCommit_ = SimpleEncCommit<typename FD::T, FD, typename FD::S>;

template <class FD>
class SummingEncCommit: public SimpleEncCommitFactory<FD>,
        public SimpleEncCommitBase_<FD>
{
    typedef bigint S;

    InteractiveProof proof;
    const FHE_PK& pk;
    const FD& FTD;
    const Player& P;
    int thread_num;

#ifdef LESS_ALLOC_MORE_MEM
    Prover<FD, Plaintext_<FD> > prover;
    Verifier<FD,S> verifier;
    Proof::Preimages preimages;
#endif

    void prepare_plaintext(PRNG& G)
    { SimpleEncCommitFactory<FD>::prepare_plaintext(G); }

public:
    map<string, Timer>& timers;

    SummingEncCommit(const Player& P, const FHE_PK& pk, const FD& FTD,
            map<string, Timer>& timers, const MachineBase& machine, int thread_num) :
    SimpleEncCommitFactory<FD>(pk, FTD, machine), SimpleEncCommitBase_<FD>(machine),
	proof(this->sec, pk, P.num_players()), pk(pk), FTD(FTD), P(P),
	thread_num(thread_num),
#ifdef LESS_ALLOC_MORE_MEM
            prover(proof, FTD), verifier(proof), preimages(proof.V, this->pk,
                FTD.get_prime(), P.num_players()),
#endif
            timers(timers) {}
    void next(Plaintext_<FD>& mess, Ciphertext& C) { SimpleEncCommitFactory<FD>::next(mess, C); }
    void create_more();
    size_t report_size(ReportType type);
    void report_size(ReportType type, MemoryUsage& res)
    { SimpleEncCommitFactory<FD>::report_size(type, res); SimpleEncCommitBase_<FD>::report_size(type, res); }
};

template <class FD>
class Multiplier;
template <class FD>
class PairwiseGenerator;

template <class FD>
class MultiEncCommit : public NonInteractiveProofSimpleEncCommit<FD>
{
    friend PairwiseGenerator<FD>;

protected:
    const vector<FHE_PK>& pks;
    const Player& P;
    PairwiseGenerator<FD>& generator;

    void prepare_plaintext(PRNG& G) { (void)G; }
    const FHE_PK& get_pk_for_verification(int offset)
    { return pks[(P.my_num() - offset + P.num_players()) % P.num_players()]; }
    void add_ciphertexts(vector<Ciphertext>& ciphertexts, int offset);

public:
    MultiEncCommit(const Player& P, const vector<FHE_PK>& pks,
            const FD& FTD,
            map<string, Timer>& timers, MachineBase& machine,
            PairwiseGenerator<FD>& generator);
};

#endif /* FHEOFFLINE_SIMPLEENCCOMMIT_H_ */
