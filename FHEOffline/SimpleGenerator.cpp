// (C) 2018 University of Bristol. See License.txt

/*
 * SimpleThread.cpp
 *
 */

#include <FHEOffline/SimpleGenerator.h>
#include "FHEOffline/SimpleMachine.h"
#include "FHEOffline/Sacrificing.h"
#include "Auth/MAC_Check.h"

template <template <class> class T, class FD>
SimpleGenerator<T,FD>::SimpleGenerator(const Names& N, const PartSetup<FD>& setup,
        const MultiplicativeMachine& machine, int thread_num, Dtype data_type) :
        GeneratorBase(thread_num, N),
        setup(setup), machine(machine), dd(P, setup),
        volatile_memory(0),
        EC(P, setup.pk, setup.FieldD, timers, machine, thread_num)
{
    if (machine.produce_inputs)
        producer = new InputProducer<FD>(P, thread_num, machine.output);
    else
        switch (data_type)
        {
        case DATA_TRIPLE:
            producer = new TripleProducer_<FD>(setup.FieldD, P.my_num(),
                    thread_num, machine.output);
            break;
        case DATA_SQUARE:
            producer = new SquareProducer<FD>(setup.FieldD, P.my_num(),
                    thread_num, machine.output);
            break;
        case DATA_BIT:
            producer = new_bit_producer(setup.FieldD, P, setup.pk, machine.get_covert(), true,
                    thread_num, machine.output);
            break;
        case DATA_INVERSE:
            producer = new InverseProducer<FD>(setup.FieldD, P.my_num(),
                    thread_num, machine.output);
            break;
        default:
            throw runtime_error("data type not implemented");
        }
}

template <template <class> class T, class FD>
SimpleGenerator<T,FD>::~SimpleGenerator()
{
    delete producer;
}

template <template <class> class T, class FD>
void SimpleGenerator<T,FD>::run()
{
    Timer timer(CLOCK_THREAD_CPUTIME_ID);
    timer.start();
    timers["MC init"].start();
    MAC_Check<typename FD::T> MC(setup.alphai);
    timers["MC init"].stop();
    while (total < machine.nTriplesPerThread or EC.has_left())
    {
        producer->run(P, setup.pk, setup.calpha, EC, dd, setup.alphai);
        producer->sacrifice(P, MC);
        total += producer->num_slots();
    }
    MC.Check(P);
    timer.stop();
    timers["Thread"] = timer;
    timers.insert(producer->timers.begin(), producer->timers.end());
    timers["Networking"] = P.timer;
}

template <template <class> class T, class FD>
size_t SimpleGenerator<T,FD>::report_size(ReportType type)
{
    return EC.report_size(type) + EC.get_volatile()
            + producer->report_size(type) + dd.report_size(type)
            + volatile_memory;
}

template <template <class> class T, class FD>
void SimpleGenerator<T,FD>::report_size(ReportType type, MemoryUsage& res)
{
    EC.report_size(type, res);
    res.add("producer", producer->report_size(type));
    res.add("generator volatile", volatile_memory);
    res.add("distributed decryption", dd.report_size(type));
}

template class SimpleGenerator<SimpleEncCommit_, FFT_Data>;
template class SimpleGenerator<SummingEncCommit, FFT_Data>;
template class SimpleGenerator<EncCommit_, FFT_Data>;

template class SimpleGenerator<SimpleEncCommit_, P2Data>;
template class SimpleGenerator<SummingEncCommit, P2Data>;
template class SimpleGenerator<EncCommit_, P2Data>;
