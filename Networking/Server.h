// (C) 2018 University of Bristol. See License.txt

/*
 * Server.h
 */

#ifndef NETWORKING_SERVER_H_
#define NETWORKING_SERVER_H_

#include "Networking/data.h"
#include "Networking/Player.h"

#include <vector>
using namespace std;

class Server
{
    vector<int> socket_num;
    vector<octet*> names;
    vector<int> ports;
    int nmachines;
    int PortnumBase;

    void get_ip(int num);
    void get_name(int num);
    void send_names(int num);

public:
    static void* start_in_thread(void* server);
    static Server* start_networking(Names& N, int my_num, int nplayers,
            string hostname = "localhost", int portnum = 9000);

    Server(int argc, char** argv);
    Server(int nmachines, int PortnumBase);
    void start();
};

#endif /* NETWORKING_SERVER_H_ */
