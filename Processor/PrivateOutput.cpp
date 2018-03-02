// (C) 2018 University of Bristol. See License.txt

/*
 * PrivateOutput.cpp
 *
 */

#include "PrivateOutput.h"
#include "Processor.h"

template<class T>
void PrivateOutput<T>::start(int player, int target, int source)
{
    T mask;
    proc.DataF.get_input(proc.get_S_ref<T>(target), mask, player);
    proc.get_S_ref<T>(target).add(proc.get_S_ref<T>(source));

    if (player == proc.P.my_num())
        masks.push_back(mask);
}

template<class T>
void PrivateOutput<T>::stop(int player, int source)
{
    if (player == proc.P.my_num())
    {
        T value;
        value.sub(proc.get_C_ref<T>(source), masks.front());
        value.output(proc.private_output, false);
        masks.pop_front();
    }
}

template class PrivateOutput<gf2n>;
template class PrivateOutput<gfp>;
