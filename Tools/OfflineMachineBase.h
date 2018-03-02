// (C) 2018 University of Bristol. See License.txt

/*
 * OfflineMachineBase.h
 *
 */

#ifndef TOOLS_OFFLINEMACHINEBASE_H_
#define TOOLS_OFFLINEMACHINEBASE_H_

#include "Tools/ezOptionParser.h"
#include "Networking/Server.h"
#include "Networking/Player.h"

class OfflineMachineBase
{
protected:
    ez::ezOptionParser opt;
    Server* server;

public:
    Names N;
    int my_num, nplayers, nthreads;
    long long ntriples, nTriplesPerThread;
    bool output;

    OfflineMachineBase();
    ~OfflineMachineBase();

    void parse_options(int argc, const char** argv);
    void start_networking_with_server(string hostname = "localhost", int portnum = 5000);
};

#endif /* TOOLS_OFFLINEMACHINEBASE_H_ */
