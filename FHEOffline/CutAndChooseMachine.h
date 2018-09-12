// (C) 2018 University of Bristol. See License.txt

/*
 * CutAndChooseMachine.h
 *
 */

#ifndef FHEOFFLINE_CUTANDCHOOSEMACHINE_H_
#define FHEOFFLINE_CUTANDCHOOSEMACHINE_H_

#include "FHEOffline/SimpleMachine.h"

class CutAndChooseMachine : public MultiplicativeMachine
{
    bool covert;

    template <class FD>
    GeneratorBase* new_generator(int i);

public:
    CutAndChooseMachine(int argc, const char** argv);

    int get_covert() const { return sec; }
};

#endif /* FHEOFFLINE_CUTANDCHOOSEMACHINE_H_ */
