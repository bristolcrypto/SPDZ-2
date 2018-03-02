// (C) 2018 University of Bristol. See License.txt

/*
 * OTMultiplier.h
 *
 */

#ifndef OT_OTMULTIPLIER_H_
#define OT_OTMULTIPLIER_H_

#include <vector>
using namespace std;

#include "OT/OTExtensionWithMatrix.h"
#include "Tools/random.h"

class NPartyTripleGenerator;

template <class T>
class OTMultiplier
{
	void multiplyForTriples(OTExtensionWithMatrix& auth_ot_ext);
	void multiplyForBits(OTExtensionWithMatrix& auth_ot_ext);
public:
    NPartyTripleGenerator& generator;
    int thread_num;
    OTExtensionWithMatrix rot_ext;
    //OTExtensionWithMatrix* auth_ot_ext;
    vector<T> c_output;
    vector< vector<T> > macs;

    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t ready;

    OTMultiplier(NPartyTripleGenerator& generator, int thread_num);
    ~OTMultiplier();
    void multiply();
};

#endif /* OT_OTMULTIPLIER_H_ */
