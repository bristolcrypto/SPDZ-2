// (C) 2018 University of Bristol. See License.txt

#include "NPartyTripleGenerator.h"

#include "OT/OTExtensionWithMatrix.h"
#include "OT/OTMultiplier.h"
#include "Math/gfp.h"
#include "Math/Share.h"
#include "Math/operators.h"
#include "Auth/Subroutines.h"
#include "Auth/MAC_Check.h"

#include <sstream>
#include <fstream>
#include <math.h>

template <class T, int N>
class Triple
{
public:
    T a[N];
    T b;
    T c[N];

    int repeat(int l)
    {
        switch (l)
        {
        case 0:
        case 2:
            return N;
        case 1:
            return 1;
        default:
            throw bad_value();
        }
    }

    T& byIndex(int l, int j)
    {
        switch (l)
        {
        case 0:
            return a[j];
        case 1:
            return b;
        case 2:
            return c[j];
        default:
            throw bad_value();
        }
    }

    template <int M>
    void amplify(const Triple<T,M>& uncheckedTriple, PRNG& G)
    {
        b = uncheckedTriple.b;
        for (int i = 0; i < N; i++)
            for (int j = 0; j < M; j++)
            {
                typename T::value_type r;
                r.randomize(G);
                a[i] += r * uncheckedTriple.a[j];
                c[i] += r * uncheckedTriple.c[j];
            }
    }

    void output(ostream& outputStream, int n = N, bool human = false)
    {
        for (int i = 0; i < n; i++)
        {
            a[i].output(outputStream, human);
            b.output(outputStream, human);
            c[i].output(outputStream, human);
        }
    }
};

template <class T, int N>
class PlainTriple : public Triple<T,N>
{
public:
    // this assumes that valueBits[1] is still set to the bits of b
    void to(vector<BitVector>& valueBits, int i)
    {
        for (int j = 0; j < N; j++)
        {
            valueBits[0].set_int128(i * N + j, this->a[j].to_m128i());
            valueBits[2].set_int128(i * N + j, this->c[j].to_m128i());
        }
    }
};

template <class T, int N>
class ShareTriple : public Triple<Share<T>, N>
{
public:
    void from(PlainTriple<T,N>& triple, vector<OTMultiplier<T>*>& ot_multipliers,
            int iTriple, const NPartyTripleGenerator& generator)
    {
        for (int l = 0; l < 3; l++)
        {
            int repeat = this->repeat(l);
            for (int j = 0; j < repeat; j++)
            {
                T value = triple.byIndex(l,j);
                T mac = value * generator.machine.get_mac_key<T>();
                for (int i = 0; i < generator.nparties-1; i++)
                    mac += ot_multipliers[i]->macs[l][iTriple * repeat + j];
                Share<T>& share = this->byIndex(l,j);
                share.set_share(value);
                share.set_mac(mac);
            }
        }
    }

    T computeCheckMAC(const T& maskedA)
    {
        return this->c[0].get_mac() - maskedA * this->b.get_mac();
    }
};

/*
 * Copies the relevant base OTs from setup
 * N.B. setup must not be stored as it will be used by other threads
 */
NPartyTripleGenerator::NPartyTripleGenerator(OTTripleSetup& setup,
        const Names& names, int thread_num, int _nTriples, int nloops,
        TripleMachine& machine) :
        globalPlayer(names, - thread_num * machine.nplayers * machine.nplayers),
        thread_num(thread_num),
        my_num(setup.get_my_num()),
        nloops(nloops),
        nparties(setup.get_nparties()),
        machine(machine)
{
    nTriplesPerLoop = DIV_CEIL(_nTriples, nloops);
    nTriples = nTriplesPerLoop * nloops;
    field_size = 128;
    nAmplify = machine.amplify ? N_AMPLIFY : 1;
    nPreampTriplesPerLoop = nTriplesPerLoop * nAmplify;

    int n = nparties;
    //baseReceiverInput = machines[0]->baseReceiverInput;
    //baseSenderInputs.resize(n-1);
    //baseReceiverOutputs.resize(n-1);
    nbase = setup.get_nbase();
    baseReceiverInput.resize(nbase);
    baseReceiverOutputs.resize(n - 1);
    baseSenderInputs.resize(n - 1);
    players.resize(n-1);

    gf2n_long::init_field(128);

    for (int i = 0; i < n-1; i++)
    {
        // i for indexing, other_player is actual number
        int other_player, id;
        if (i >= my_num)
            other_player = i + 1;
        else
            other_player = i;

        // copy base OT inputs + outputs
        for (int j = 0; j < 128; j++)
        {
            baseReceiverInput.set_bit(j, (unsigned int)setup.get_base_receiver_input(j));
        }
        baseReceiverOutputs[i] = setup.baseOTs[i]->receiver_outputs;
        baseSenderInputs[i] = setup.baseOTs[i]->sender_inputs;

        // new TwoPartyPlayer with unique id for each thread + pair of players
        if (my_num < other_player)
            id = (thread_num+1)*n*n + my_num*n + other_player;
        else
            id = (thread_num+1)*n*n + other_player*n + my_num;
        players[i] = new TwoPartyPlayer(names, other_player, id);
        cout << "Set up with player " << other_player << " in thread " << thread_num << " with id " << id << endl;
    }

    pthread_mutex_init(&mutex, 0);
    pthread_cond_init(&ready, 0);
}

NPartyTripleGenerator::~NPartyTripleGenerator()
{
    for (size_t i = 0; i < players.size(); i++)
        delete players[i];
    //delete nplayer;
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&ready);
}

template<class T>
void* run_ot_thread(void* ptr)
{
    ((OTMultiplier<T>*)ptr)->multiply();
    return NULL;
}

template<class T>
void NPartyTripleGenerator::generate()
{
    vector< OTMultiplier<T>* > ot_multipliers(nparties-1);

    timers["Generator thread"].start();

    for (int i = 0; i < nparties-1; i++)
    {
        ot_multipliers[i] = new OTMultiplier<T>(*this, i);
        pthread_mutex_lock(&ot_multipliers[i]->mutex);
        pthread_create(&(ot_multipliers[i]->thread), 0, run_ot_thread<T>, ot_multipliers[i]);
    }

    // add up the shares from each thread and write to file
    stringstream ss;
    ss << machine.prep_data_dir;
    if (machine.generateBits)
        ss << "Bits-";
    else
        ss << "Triples-";
    ss << T::type_char() << "-P" << my_num;
    if (thread_num != 0)
        ss << "-" << thread_num;
    ofstream outputFile;
    if (machine.output)
        outputFile.open(ss.str().c_str());

    if (machine.generateBits)
    	generateBits(ot_multipliers, outputFile);
    else
    	generateTriples(ot_multipliers, outputFile);

    timers["Generator thread"].stop();
    if (machine.output)
        cout << "Written " << nTriples << " outputs to " << ss.str() << endl;
    else
        cout << "Generated " << nTriples << " outputs" << endl;

    // wait for threads to finish
    for (int i = 0; i < nparties-1; i++)
    {
        pthread_mutex_unlock(&ot_multipliers[i]->mutex);
        pthread_join(ot_multipliers[i]->thread, NULL);
        cout << "OT thread " << i << " finished\n" << flush;
    }
    cout << "OT threads finished\n";

    for (size_t i = 0; i < ot_multipliers.size(); i++)
        delete ot_multipliers[i];
}

template<>
void NPartyTripleGenerator::generateBits(vector< OTMultiplier<gf2n>* >& ot_multipliers,
		ofstream& outputFile)
{
    PRNG share_prg;
    share_prg.ReSeed();

    int nBitsToCheck = nTriplesPerLoop + field_size;
    valueBits.resize(1);
    valueBits[0].resize(ceil(1.0 * nBitsToCheck / field_size) * field_size);
    MAC_Check<gf2n> MC(machine.get_mac_key<gf2n>());
    vector< Share<gf2n> > bits(nBitsToCheck);
    vector< Share<gf2n> > to_open(1);
    vector<gf2n> opened(1);

    start_progress(ot_multipliers);

    for (int k = 0; k < nloops; k++)
    {
        print_progress(k);

    	valueBits[0].randomize_blocks<gf2n>(share_prg);

        for (int i = 0; i < nparties-1; i++)
            pthread_cond_signal(&ot_multipliers[i]->ready);
        timers["Authentication OTs"].start();
        for (int i = 0; i < nparties-1; i++)
            pthread_cond_wait(&ot_multipliers[i]->ready, &ot_multipliers[i]->mutex);
        timers["Authentication OTs"].stop();

        octet seed[SEED_SIZE];
        Create_Random_Seed(seed, globalPlayer, SEED_SIZE);
        PRNG G;
        G.SetSeed(seed);

        Share<gf2n> check_sum;
        gf2n r;
        for (int j = 0; j < nBitsToCheck; j++)
        {
            gf2n mac_sum = bool(valueBits[0].get_bit(j)) * machine.get_mac_key<gf2n>();
            for (int i = 0; i < nparties-1; i++)
                mac_sum += ot_multipliers[i]->macs[0][j];
            bits[j].set_share(valueBits[0].get_bit(j));
            bits[j].set_mac(mac_sum);
            r.randomize(G);
            check_sum += r * bits[j];
        }

        to_open[0] = check_sum;
        MC.POpen_Begin(opened, to_open, globalPlayer);
        MC.POpen_End(opened, to_open, globalPlayer);
        MC.Check(globalPlayer);

        if (machine.output)
            for (int j = 0; j < nTriplesPerLoop; j++)
                bits[j].output(outputFile, false);

        for (int i = 0; i < nparties-1; i++)
            pthread_cond_signal(&ot_multipliers[i]->ready);
   }
}

template<>
void NPartyTripleGenerator::generateBits(vector< OTMultiplier<gfp>* >& ot_multipliers,
		ofstream& outputFile)
{
	generateTriples(ot_multipliers, outputFile);
}

template<class T>
void NPartyTripleGenerator::generateTriples(vector< OTMultiplier<T>* >& ot_multipliers,
    ofstream& outputFile)
{
	PRNG share_prg;
	share_prg.ReSeed();

    valueBits.resize(3);
    for (int i = 0; i < 2; i++)
        valueBits[2*i].resize(field_size * nPreampTriplesPerLoop);
    valueBits[1].resize(field_size * nTriplesPerLoop);
    vector< PlainTriple<T,N_AMPLIFY> > preampTriples;
    vector< PlainTriple<T,2> > amplifiedTriples;
    vector< ShareTriple<T,2> > uncheckedTriples;
    MAC_Check<T> MC(machine.get_mac_key<T>());

    if (machine.amplify)
        preampTriples.resize(nTriplesPerLoop);
    if (machine.generateMACs)
      {
	amplifiedTriples.resize(nTriplesPerLoop);
	uncheckedTriples.resize(nTriplesPerLoop);
      }

    start_progress(ot_multipliers);

    for (int k = 0; k < nloops; k++)
    {
        print_progress(k);

        for (int j = 0; j < 2; j++)
            valueBits[j].randomize_blocks<T>(share_prg);

        timers["OTs"].start();
        for (int i = 0; i < nparties-1; i++)
            pthread_cond_wait(&ot_multipliers[i]->ready, &ot_multipliers[i]->mutex);
        timers["OTs"].stop();

        for (int j = 0; j < nPreampTriplesPerLoop; j++)
        {
            T a(valueBits[0].get_int128(j));
            T b(valueBits[1].get_int128(j / nAmplify));
            T c = a * b;
            timers["Triple computation"].start();
            for (int i = 0; i < nparties-1; i++)
            {
                c += ot_multipliers[i]->c_output[j];
            }
            timers["Triple computation"].stop();
            if (machine.amplify)
            {
                preampTriples[j/nAmplify].a[j%nAmplify] = a;
                preampTriples[j/nAmplify].b = b;
                preampTriples[j/nAmplify].c[j%nAmplify] = c;
            }
            else if (machine.output)
            {
                timers["Writing"].start();
                a.output(outputFile, false);
                b.output(outputFile, false);
                c.output(outputFile, false);
                timers["Writing"].stop();
            }
        }

        if (machine.amplify)
        {
            octet seed[SEED_SIZE];
            Create_Random_Seed(seed, globalPlayer, SEED_SIZE);
            PRNG G;
            G.SetSeed(seed);
            for (int iTriple = 0; iTriple < nTriplesPerLoop; iTriple++)
            {
                PlainTriple<T,2> triple;
                triple.amplify(preampTriples[iTriple], G);

                if (machine.generateMACs)
                    amplifiedTriples[iTriple] = triple;
                else if (machine.output)
                {
                    timers["Writing"].start();
                    triple.output(outputFile);
                    timers["Writing"].stop();
                }
            }

            if (machine.generateMACs)
            {
                for (int iTriple = 0; iTriple < nTriplesPerLoop; iTriple++)
                    amplifiedTriples[iTriple].to(valueBits, iTriple);

                for (int i = 0; i < nparties-1; i++)
                    pthread_cond_signal(&ot_multipliers[i]->ready);
                timers["Authentication OTs"].start();
                for (int i = 0; i < nparties-1; i++)
                    pthread_cond_wait(&ot_multipliers[i]->ready, &ot_multipliers[i]->mutex);
                timers["Authentication OTs"].stop();

                for (int iTriple = 0; iTriple < nTriplesPerLoop; iTriple++)
                {
                    uncheckedTriples[iTriple].from(amplifiedTriples[iTriple], ot_multipliers, iTriple, *this);

                    if (!machine.check and machine.output)
                    {
                        timers["Writing"].start();
                        amplifiedTriples[iTriple].output(outputFile);
                        timers["Writing"].stop();
                    }
                }

                if (machine.check)
                {
                    vector< Share<T> > maskedAs(nTriplesPerLoop);
                    vector< ShareTriple<T,1> > maskedTriples(nTriplesPerLoop);
                    for (int j = 0; j < nTriplesPerLoop; j++)
                    {
                        maskedTriples[j].amplify(uncheckedTriples[j], G);
                        maskedAs[j] = maskedTriples[j].a[0];
                    }

                    vector<T> openedAs(nTriplesPerLoop);
                    MC.POpen_Begin(openedAs, maskedAs, globalPlayer);
                    MC.POpen_End(openedAs, maskedAs, globalPlayer);

                    for (int j = 0; j < nTriplesPerLoop; j++)
                        MC.AddToCheck(maskedTriples[j].computeCheckMAC(openedAs[j]), int128(0), globalPlayer);

                    MC.Check(globalPlayer);

                    if (machine.generateBits)
                        generateBitsFromTriples(uncheckedTriples, MC, outputFile);
                    else
                        if (machine.output)
                            for (int j = 0; j < nTriplesPerLoop; j++)
                                uncheckedTriples[j].output(outputFile, 1);
                }
            }
        }

        for (int i = 0; i < nparties-1; i++)
            pthread_cond_signal(&ot_multipliers[i]->ready);
    }
}

template<>
void NPartyTripleGenerator::generateBitsFromTriples(
        vector< ShareTriple<gfp,2> >& triples, MAC_Check<gfp>& MC, ofstream& outputFile)
{
    vector< Share<gfp> > a_plus_b(nTriplesPerLoop), a_squared(nTriplesPerLoop);
    for (int i = 0; i < nTriplesPerLoop; i++)
        a_plus_b[i] = triples[i].a[0] + triples[i].b;
    vector<gfp> opened(nTriplesPerLoop);
    MC.POpen_Begin(opened, a_plus_b, globalPlayer);
    MC.POpen_End(opened, a_plus_b, globalPlayer);
    for (int i = 0; i < nTriplesPerLoop; i++)
        a_squared[i] = triples[i].a[0] * opened[i] - triples[i].c[0];
    MC.POpen_Begin(opened, a_squared, globalPlayer);
    MC.POpen_End(opened, a_squared, globalPlayer);
    Share<gfp> one(gfp(1), globalPlayer.my_num(), MC.get_alphai());
    for (int i = 0; i < nTriplesPerLoop; i++)
    {
        gfp root = opened[i].sqrRoot();
        if (root.is_zero())
            continue;
        Share<gfp> bit = (triples[i].a[0] / root + one) / gfp(2);
        if (machine.output)
            bit.output(outputFile, false);
    }
}

template<>
void NPartyTripleGenerator::generateBitsFromTriples(
        vector< ShareTriple<gf2n,2> >& triples, MAC_Check<gf2n>& MC, ofstream& outputFile)
{
    throw how_would_that_work();
    // warning gymnastics
    triples[0];
    MC.number();
    outputFile << "";
}

template <class T>
void NPartyTripleGenerator::start_progress(vector< OTMultiplier<T>* >& ot_multipliers)
{
    for (int i = 0; i < nparties-1; i++)
        pthread_cond_wait(&ot_multipliers[i]->ready, &ot_multipliers[i]->mutex);
    lock();
    signal();
    wait();
    gettimeofday(&last_lap, 0);
    for (int i = 0; i < nparties-1; i++)
        pthread_cond_signal(&ot_multipliers[i]->ready);
}

void NPartyTripleGenerator::print_progress(int k)
{
    if (thread_num == 0 && my_num == 0)
    {
        struct timeval stop;
        gettimeofday(&stop, 0);
        if (timeval_diff_in_seconds(&last_lap, &stop) > 1)
        {
            double diff = timeval_diff_in_seconds(&machine.start, &stop);
            double throughput = k * nTriplesPerLoop * machine.nthreads / diff;
            double remaining = diff * (nloops - k) / k;
            cout << k << '/' << nloops << ", throughput: " << throughput
                    << ", time left: " << remaining << ", elapsed: " << diff
                    << ", estimated total: " << (diff + remaining) << endl;
            last_lap = stop;
        }
    }
}

void NPartyTripleGenerator::lock()
{
    pthread_mutex_lock(&mutex);
}

void NPartyTripleGenerator::unlock()
{
    pthread_mutex_unlock(&mutex);
}

void NPartyTripleGenerator::signal()
{
    pthread_cond_signal(&ready);
}

void NPartyTripleGenerator::wait()
{
    pthread_cond_wait(&ready, &mutex);
}


template void NPartyTripleGenerator::generate<gf2n>();
template void NPartyTripleGenerator::generate<gfp>();
