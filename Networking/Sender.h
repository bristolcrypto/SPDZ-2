// (C) 2018 University of Bristol. See License.txt

/*
 * Sender.h
 *
 */

#ifndef NETWORKING_SENDER_H_
#define NETWORKING_SENDER_H_

#include <pthread.h>

#include "Tools/octetStream.h"
#include "Tools/WaitQueue.h"
#include "Tools/time-func.h"

class Sender
{
    int socket;
    WaitQueue<const octetStream*> in;
    WaitQueue<const octetStream*> out;
    pthread_t thread;

    // prevent copying
    Sender(const Sender& other);

public:
    Timer timer;

    Sender(int socket);

    void start();
    void stop();
    void run();

    void request(const octetStream& os);
    void wait(const octetStream& os);
};

#endif /* NETWORKING_SENDER_H_ */
