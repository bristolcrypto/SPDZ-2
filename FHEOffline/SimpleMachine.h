// (C) 2018 University of Bristol. See License.txt

/*
 * SimpleMachine.h
 *
 */

#ifndef FHEOFFLINE_SIMPLEMACHINE_H_
#define FHEOFFLINE_SIMPLEMACHINE_H_

#include "FHE/FHE_Keys.h"
#include "FHEOffline/DataSetup.h"
#include "Networking/Player.h"
#include "FHEOffline/SimpleGenerator.h"
#include "Tools/OfflineMachineBase.h"

class MachineBase : public OfflineMachineBase
{
protected:
    vector<GeneratorBase*> generators;
    string hostname;
    Timer timer;
    pthread_t throughput_loop_thread;
    int portnum_base;
    Dtype data_type;

public:
    int sec;
    int field_size;
    int extra_slack;
    bool produce_inputs;
    bool use_gf2n;

    MachineBase();
    MachineBase(int argc, const char** argv);
    void run();

    void parse_options(int argc, const char** argv);

    string item_type();

    void throughput_loop();
    static void* run_throughput_loop(void* machine);

    size_t report_size(ReportType type);

    string tradeoff();

    void mult_performance();
};

class MultiplicativeMachine : public MachineBase
{
protected:
    DataSetup setup;

    void parse_options(int argc, const char** argv);

    void generate_setup(int slack);
    template <class FD>
    void fake_keys(int slack);

public:
    virtual ~MultiplicativeMachine() {}

    virtual int get_covert() const { return 0; }
};

class SimpleMachine : public MultiplicativeMachine
{
    template <template <class FD> class EC, class FD>
    GeneratorBase* new_generator(int i);

public:
    SimpleMachine(int argc, const char** argv);
};

#endif /* FHEOFFLINE_SIMPLEMACHINE_H_ */
