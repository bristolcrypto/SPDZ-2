// (C) 2018 University of Bristol. See License.txt

/*
 * CutAndChooseMachine.cpp
 *
 */

#include "FHEOffline/CutAndChooseMachine.h"
#include "FHEOffline/SimpleGenerator.h"

CutAndChooseMachine::CutAndChooseMachine(int argc, const char** argv)
{
    opt.add(
          "", // Default.
          0, // Required?
          0, // Number of args expected.
          0, // Delimiter if expecting multiple args.
          "Use covert security (default: active; use -s for parameter)", // Help description.
          "-c", // Flag token.
          "--covert" // Flag token.
    );
    parse_options(argc, argv);
    covert = opt.isSet("--covert");
    if (not covert and sec != 40)
        throw runtime_error("active cut-and-choose only implemented for 40-bit security");
    if (covert)
        generate_setup(COVERT_SPDZ2_SLACK);
    else
        generate_setup(ACTIVE_SPDZ2_SLACK);
    for (int i = 0; i < nthreads; i++)
    {
        if (use_gf2n)
            generators.push_back(new_generator<P2Data>(i));
        else
            generators.push_back(new_generator<FFT_Data>(i));
    }
}

template <class FD>
GeneratorBase* CutAndChooseMachine::new_generator(int i)
{
    SimpleGenerator<EncCommit_, FD>* generator =
        new SimpleGenerator<EncCommit_, FD>(N, setup.part<FD>(), *this, i, data_type);
    if (covert)
        generator->EC.init(generator->P, setup.part<FD>().pk, sec, Full);
    else
        generator->EC.init(generator->P, setup.part<FD>().pk, Full, setup.part<FD>().FieldD, i);
    return generator;
}
