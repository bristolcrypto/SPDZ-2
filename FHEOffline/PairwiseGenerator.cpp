// (C) 2018 University of Bristol. See License.txt

/*
 * PairwiseGenerator.cpp
 *
 */

#include "FHEOffline/PairwiseGenerator.h"
#include "FHEOffline/PairwiseMachine.h"
#include "FHEOffline/Producer.h"

template <class FD>
PairwiseGenerator<FD>::PairwiseGenerator(int thread_num,
        PairwiseMachine& machine) :
    GeneratorBase(thread_num, machine.N),
    producer(machine.setup<FD>().FieldD, machine.N.my_num(),
            thread_num, machine.output),
    EC(P, machine.other_pks, machine.setup<FD>().FieldD, timers, machine, *this),
    C(machine.sec, machine.setup<FD>().params), volatile_memory(0),
    machine(machine), global_player(machine.N, (1LL << 28) + (thread_num << 16))
{
    for (int i = 1; i < machine.N.num_players(); i++)
        multipliers.push_back(new Multiplier<FD>(i, *this));
    const FD& FieldD = machine.setup<FD>().FieldD;
    a.resize(machine.sec, FieldD);
    b.resize(machine.sec, FieldD);
    c.resize(machine.sec, FieldD);
    a.allocate_slots(FieldD.get_prime());
    b.allocate_slots(FieldD.get_prime());
    // extra limb for addition
    c.allocate_slots((bigint)FieldD.get_prime() << 64);
    b_mod_q.resize(machine.sec,
    { machine.setup<FD>().params, evaluation, evaluation });
}

template <class FD>
PairwiseGenerator<FD>::~PairwiseGenerator()
{
    for (auto m : multipliers)
        delete m;
}

template <class FD>
void PairwiseGenerator<FD>::run()
{
    PRNG G;
    G.ReSeed();
    MAC_Check<typename FD::T> MC(machine.setup<FD>().alphai);

    while (total < machine.nTriplesPerThread)
    {
        timers["Randomization"].start();
        a.randomize(G);
        b.randomize(G);
        timers["Randomization"].stop();
        size_t prover_memory = EC.generate_proof(C, a, ciphertexts, cleartexts);
        timers["Plaintext multiplication"].start();
        c.mul(a, b);
        timers["Plaintext multiplication"].stop();
        timers["FFT of b"].start();
        for (int i = 0; i < machine.sec; i++)
            b_mod_q.at(i).from_vec(b.at(i).get_poly());
        timers["FFT of b"].stop();
        timers["Proof exchange"].start();
        size_t verifier_memory = EC.create_more(ciphertexts, cleartexts);
        timers["Proof exchange"].stop();
        volatile_memory = max(prover_memory, verifier_memory);

        Rq_Element values({machine.setup<FD>().params, evaluation, evaluation});
        for (int k = 0; k < machine.sec; k++)
        {
            producer.ai = a[k];
            producer.bi = b[k];
            producer.ci = c[k];

            for (int j = 0; j < 3; j++)
            {
                timers["Plaintext multiplication"].start();
                producer.macs[j].mul(machine.setup<FD>().alpha, producer.values[j]);
                timers["Plaintext multiplication"].stop();

                if (j == 1)
                    values = b_mod_q[k];
                else
                {
                    timers["Plaintext conversion"].start();
                    values.from_vec(producer.values[j].get_poly());
                    timers["Plaintext conversion"].stop();
                }

                for (auto m : multipliers)
                    m->multiply_alpha_and_add(producer.macs[j], values);
            }
            producer.reset();
            total += producer.sacrifice(P, MC);
        }

        timers["Checking"].start();
        MC.Check(P);
        timers["Checking"].stop();
    }

    cout << "Could save " << 1e-9 * a.report_size(CAPACITY) << " GB" << endl;
    timers.insert(EC.timers.begin(), EC.timers.end());
    timers.insert(producer.timers.begin(), producer.timers.end());
    timers["Networking"] = P.timer;
}

template <class FD>
size_t PairwiseGenerator<FD>::report_size(ReportType type)
{
    size_t res = a.report_size(type) + b.report_size(type) + c.report_size(type);
    for (auto m : multipliers)
        res += m->report_size(type);
    res += producer.report_size(type);
    res += multipliers[0]->report_volatile();
    res += volatile_memory + C.report_size(type);
    res += ciphertexts.get_max_length() + cleartexts.get_max_length();
    res += EC.report_size(type) + EC.volatile_memory;
    res += b_mod_q.report_size(type);
    return res;
}

template <class FD>
size_t PairwiseGenerator<FD>::report_sent()
{
    return P.sent + global_player.sent;
}

template <class FD>
void PairwiseGenerator<FD>::report_size(ReportType type, MemoryUsage& res)
{
    multipliers[0]->report_size(type, res);
    res.add("shares",
            a.report_size(type) + b.report_size(type) + c.report_size(type));
    res.add("producer", producer.report_size(type));
    res.add("my ciphertexts", C.report_size(CAPACITY));
    res.add("serialized ciphertexts", ciphertexts.get_max_length());
    res.add("serialized cleartexts", cleartexts.get_max_length());
    res.add("generator volatile", volatile_memory);
    res.add("b mod p", b_mod_q.report_size(type));
    res += EC.memory_usage;
}

template class PairwiseGenerator<FFT_Data>;
template class PairwiseGenerator<P2Data>;
