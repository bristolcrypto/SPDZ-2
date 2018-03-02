// (C) 2018 University of Bristol. See License.txt

/*
 * PrivateOutput.h
 *
 */

#ifndef PROCESSOR_PRIVATEOUTPUT_H_
#define PROCESSOR_PRIVATEOUTPUT_H_

#include <deque>
using namespace std;

#include "Math/Share.h"

class Processor;

template<class T>
class PrivateOutput
{
    Processor& proc;
    deque<T> masks;

public:
    PrivateOutput(Processor& proc) : proc(proc) { };

    void start(int player, int target, int source);
    void stop(int player, int source);
};

#endif /* PROCESSOR_PRIVATEOUTPUT_H_ */
