// (C) 2016 University of Bristol. See License.txt

/*
 * ServerSocket.h
 *
 */

#ifndef NETWORKING_SERVERSOCKET_H_
#define NETWORKING_SERVERSOCKET_H_

#include <map>
#include <set>
using namespace std;

#include <pthread.h>

#include "Tools/WaitQueue.h"
#include "Tools/Signal.h"

class ServerSocket
{
    int main_socket, portnum;
    map<int,int> clients;
    set<int> used;
    Signal data_signal;
    pthread_t thread;

    // disable copying
    ServerSocket(const ServerSocket& other);

public:
    ServerSocket(int Portnum);
    ~ServerSocket();

    void accept_clients();

    // This depends on clients sending their id as int.
    // Has to be thread-safe.
    int get_connection_socket(int number);

    void close_socket();
};

#endif /* NETWORKING_SERVERSOCKET_H_ */
