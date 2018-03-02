// (C) 2018 University of Bristol. See License.txt

/*
 * ServerSocket.h
 *
 */

#ifndef NETWORKING_SERVERSOCKET_H_
#define NETWORKING_SERVERSOCKET_H_

#include <map>
#include <set>
 #include <queue>
using namespace std;

#include <pthread.h>

#include "Tools/WaitQueue.h"
#include "Tools/Signal.h"

class ServerSocket
{
protected:
    int main_socket, portnum;
    map<int,int> clients;
    std::set<int> used;
    Signal data_signal;
    pthread_t thread;

    // disable copying
    ServerSocket(const ServerSocket& other);

    // receive id from client
    int assign_client_id(int consocket);

public:
    ServerSocket(int Portnum);
    virtual ~ServerSocket();

    virtual void init();

    virtual void accept_clients();

    // This depends on clients sending their id as int.
    // Has to be thread-safe.
    int get_connection_socket(int number);

    // How many client connections have been made.
    virtual int get_connection_count();

    void close_socket();
};

/*
 * ServerSocket where clients do not send any identifiers upon connecting.
 */
class AnonymousServerSocket : public ServerSocket
{
private:
    // Global no. of client sockets that have been returned - used to create identifiers
    static int global_client_socket_count;
    // No. of accepted connections in this instance
    int num_accepted_clients;
    queue<int> client_connection_queue;

public:
    AnonymousServerSocket(int Portnum) :
        ServerSocket(Portnum), num_accepted_clients(0) { };
    // override so clients do not send id
    void accept_clients();
    void init();

    virtual int get_connection_count();

    // Get socket for the last client who connected
    // Writes a unique client identifier (i.e. a counter) to client_id
    int get_connection_socket(int& client_id);
};

#endif /* NETWORKING_SERVERSOCKET_H_ */
