// (C) 2018 University of Bristol. See License.txt

/*
 * benchmarking.h
 *
 */

#ifndef TOOLS_BENCHMARKING_H_
#define TOOLS_BENCHMARKING_H_

#include <stdexcept>

// call before insecure benchmarking functionality
inline void insecure(string message, bool warning = true)
{
#ifdef INSECURE
    if (warning)
        cerr << "WARNING: insecure " << message << endl;
#else
    (void)warning;
    string msg = "You are trying to use insecure benchmarking functionality for "
            + message + ".\nYou can activate this at compile time "
                    "by adding -DINSECURE to the compiler options.\n"
                    "Make sure to run make clean as well.";
    throw runtime_error(msg);
#endif
}

#endif /* TOOLS_BENCHMARKING_H_ */
