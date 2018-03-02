// (C) 2018 University of Bristol. See License.txt

/*
 * spdz2-offline.cpp
 *
 */

#include <signal.h>
#include <stdexcept>
using namespace std;

#include "FHEOffline/DataSetup.h"
#include "FHEOffline/DistKeyGen.h"
#include "FHEOffline/EncCommit.h"
#include "FHEOffline/Producer.h"
#include "Networking/Server.h"
#include "FHE/NTL-Subs.h"
#include "Tools/ezOptionParser.h"
#include "Tools/mkpath.h"
#include "Math/Setup.h"

class Spdz2
{
public:
    int sec;
    int covert;
    Names N;
    bool stop_requested;
    DataSetup setup;
    int prime_length, gf2n_length;

    Spdz2() : sec(40), covert(2), stop_requested(false),
            prime_length(128), gf2n_length(40) {}

    void stop()
    {
        cout << "Stopping production..." << endl;
        stop_requested = true;
    }
};

template <class FD>
class Spdz2SetupThread;

template <class FD>
class Spdz2GeneratorThread
{
public:
    Spdz2SetupThread<FD>& setup_thread;
    Producer<FD>& producer;
    pthread_t thread;
    int id;

    Spdz2GeneratorThread(Spdz2SetupThread<FD>& setup, Producer<FD>& producer, int id) :
            setup_thread(setup), producer(producer), thread(0), id(id)
    {
    }

    void* run()
    {
        PartSetup<FD>& setup = setup_thread.setup;
        const Spdz2& spdz2 = setup_thread.spdz2;
        Player P(spdz2.N, ((id + 1) << 8) + FD::T::type_char());
        EncCommit_<FD> EC(P, setup.pk, spdz2.covert, producer.required_randomness());
        DistDecrypt<FD> dd(P, setup.sk, setup.pk, setup.FieldD);
        MAC_Check<typename FD::T> MC(setup.alphai);
        string data_type = producer.data_type();
        transform(data_type.begin(), data_type.end(), data_type.begin(), ::tolower);
        cout << "Starting to produce " << FD::T::type_string() << " " << data_type << endl;
        int total = 0;
        Timer timer;
        timer.start();
        vector<octetStream> os(P.num_players());
        while (true)
        {
            bool stop = false;
            // see if all happy to continue
            os[P.my_num()].reset_write_head();
            os[P.my_num()].store_int(spdz2.stop_requested, 1);
            P.Broadcast_Receive(os);
            for (auto& o : os)
                if(o.get_int(1))
                    stop = true;
            if (stop)
                break;

            producer.run(P, setup.pk, setup.calpha, EC, dd, setup.alphai);
            producer.sacrifice(P, MC);
            total += producer.num_slots();
            cout << "Produced " << total << " " << FD::T::type_string() << " "
                    << data_type << ", " << total / timer.elapsed()
                    << " per second" << endl;
        }
        MC.Check(P);
        cout << "Finished producing " << FD::T::type_string() << " " << data_type << endl;
        return 0;
    }
};

template <class FD>
void* run_producer(void* thread)
{
    return ((Spdz2GeneratorThread<FD>*)thread)->run();
}

template <class FD>
class Spdz2SetupThread
{
public:
    PartSetup<FD> setup;
    int plaintext_length;
    const Spdz2& spdz2;
    Signal signal;

    Spdz2SetupThread(int plaintext_length, const Spdz2& spdz2) :
            plaintext_length(plaintext_length), spdz2(spdz2)
    {
        signal.lock();
    }

    void* run()
    {
        Player P(spdz2.N, FD::T::type_char());
        setup.generate_setup(P.num_players(), plaintext_length, spdz2.sec, 0, false);
        Run_Gen_Protocol(setup.pk, setup.sk, P, spdz2.covert, false);
        generate_mac_key(setup.alphai, setup.calpha, setup.FieldD, setup.pk, P, spdz2.covert);
        signal.lock();
        signal.broadcast();
        signal.unlock();

        string dir = get_prep_dir(P.num_players(), spdz2.prime_length, spdz2.gf2n_length);
        Producer<FD>* producers[] =
        {
            new TripleProducer_<FD>(setup.FieldD, P.my_num(), 0, true, dir),
            new SquareProducer<FD>(setup.FieldD, P.my_num(), 0, true, dir),
            new_bit_producer(setup.FieldD, P, setup.pk, spdz2.covert, true, 0, true, dir),
            new InverseProducer<FD>(setup.FieldD, P.my_num(), 0, true, true, dir),
            new InputProducer<FD>(P, 0, true, dir)
        };
        vector<Spdz2GeneratorThread<FD>*> generators;
        for (int i = 0; i < 5; i++)
        {
            generators.push_back(new Spdz2GeneratorThread<FD>(*this, *producers[i], i));
            pthread_create(&generators[i]->thread, 0, run_producer<FD>, generators[i]);
        }
        for (int i = 0; i < 5; i++)
        {
            pthread_join(generators[i]->thread, 0);
            delete generators[i];
            delete producers[i];
        }
        return 0;
    }
};

template <class FD>
void* run_setup(void* setup)
{
    return ((Spdz2SetupThread<FD>*)setup)->run();
}

void handler(int signum)
{
    (void)signum;
}

void* handle_interrupt(void* spdz2)
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handler;
    sigaction(SIGINT, &action, 0);
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_UNBLOCK, &sigset, 0);
    pause();
    ((Spdz2*)spdz2)->stop();
    return 0;
}

int main(int argc, const char** argv)
{
    ez::ezOptionParser opt;
    opt.add(
            "2", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "Number of parties (default: 2).", // Help description.
            "-N", // Flag token.
            "--nparties" // Flag token.
    );
    opt.add(
            "", // Default.
            1, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "This player's number, starting with 0 (required).", // Help description.
            "-p", // Flag token.
            "--player" // Flag token.
    );
    opt.add(
            "localhost", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "Host where party 0 is running (default: localhost)", // Help description.
            "-h", // Flag token.
            "--hostname" // Flag token.
    );
    opt.add(
            "5000", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "Base port number (default: 5000).", // Help description.
            "-pn", // Flag token.
            "--portnum" // Flag token.
    );
    opt.add(
            "128", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "Bit length of prime modulus (default: 128)", // Help description.
            "-f", // Flag token.
            "--field-size" // Flag token.
    );
    opt.add(
            "10", // Default.
            0, // Required?
            1, // Number of args expected.
            0, // Delimiter if expecting multiple args.
            "Covert security parameter (default: 10)", // Help description.
            "-c", // Flag token.
            "--covert" // Flag token.
    );
    opt.parse(argc, argv);
    if (!opt.isSet("-p"))
    {
        string usage;
        opt.getUsage(usage);
        cout << usage;
        exit(0);
    }

    int my_num, nplayers;
    int portnum_base;
    string hostname;
    Spdz2 spdz2;
    opt.get("-p")->getInt(my_num);
    opt.get("-N")->getInt(nplayers);
    opt.get("-f")->getInt(spdz2.prime_length);
    opt.get("-pn")->getInt(portnum_base);
    opt.get("-h")->getString(hostname);
    opt.get("-c")->getInt(spdz2.covert);

    if(mkdir_p(PREP_DIR) == -1)
    {
        throw runtime_error(
                (string) "cannot use " + PREP_DIR
                        + " (set another PREP_DIR in CONFIG if needed)");
    }

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigprocmask(SIG_BLOCK, &sigset, 0);
    pthread_t interrupt_thread;
    pthread_create(&interrupt_thread, 0, handle_interrupt, &spdz2);

    Server* server = Server::start_networking(spdz2.N, my_num, nplayers,
            hostname, portnum_base);
    Spdz2SetupThread<FFT_Data> thread_p(spdz2.prime_length, spdz2);
    Spdz2SetupThread<P2Data> thread_2(spdz2.gf2n_length, spdz2);
    pthread_t threads[2];
    pthread_create(&threads[0], 0, run_setup<FFT_Data>, &thread_p);
    pthread_create(&threads[1], 0, run_setup<P2Data>, &thread_2);

    // gfp parameter generation is much faster
    thread_p.signal.wait();
    DataSetup& setup = spdz2.setup;
    setup.setup_p = thread_p.setup;
    // write preliminary data for early checking
    setup.write_setup(spdz2.N, true);
    setup.output(spdz2.N.my_num(), spdz2.N.num_players(), true);

    // gf2n is slower
    thread_2.signal.wait();
    setup.setup_2 = thread_2.setup;
    setup.write_setup(spdz2.N, false);
    setup.output(spdz2.N.my_num(), spdz2.N.num_players(), true);

    for (int i = 0; i < 2; i++)
        pthread_join(threads[i], 0);
    pthread_cancel(interrupt_thread);

    if (server)
      delete server;
}
