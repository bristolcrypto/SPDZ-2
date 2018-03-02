// (C) 2018 University of Bristol. See License.txt

/*
 * SimpleThread.h
 *
 */

#ifndef FHEOFFLINE_SIMPLEGENERATOR_H_
#define FHEOFFLINE_SIMPLEGENERATOR_H_

#include "Networking/Player.h"
#include "FHEOffline/SimpleEncCommit.h"
#include "FHEOffline/DataSetup.h"
#include "FHEOffline/Producer.h"
#include "FHEOffline/SimpleDistDecrypt.h"
#include "Processor/Data_Files.h"

class SimpleMachine;
class MultiplicativeMachine;

class GeneratorBase
{
protected:
    int thread_num;

public:
    Player P;
    pthread_t thread;
    long long total;

    map<string, Timer> timers;

    GeneratorBase(int thread_num, const Names& N) :
        thread_num(thread_num), P(N, thread_num << 16), thread(0), total(0) {}
    virtual ~GeneratorBase() {}
    virtual void run() = 0;
    virtual size_t report_size(ReportType type) = 0;
    virtual void report_size(ReportType type, MemoryUsage& res) = 0;
    virtual size_t report_sent() = 0;

    int get_thread_num() const { return thread_num; }
};

template <template <class FD> class T, class FD>
class SimpleGenerator : public GeneratorBase
{
    const PartSetup<FD>& setup;
    const MultiplicativeMachine& machine;

    SimpleDistDecrypt<FD> dd;

    size_t volatile_memory;

public:
    T<FD> EC;
    Producer<FD>* producer;

    SimpleGenerator(const Names& N, const PartSetup<FD>& setup,
            const MultiplicativeMachine& machine, int thread_num,
            Dtype data_type = DATA_TRIPLE);
    ~SimpleGenerator();

    void run();
    size_t report_size(ReportType type);
    void report_size(ReportType type, MemoryUsage& res);
    size_t report_sent() { return P.sent; }
};

#endif /* FHEOFFLINE_SIMPLEGENERATOR_H_ */
