// (C) 2018 University of Bristol. See License.txt

/*
 * Receiver.h
 *
 */

#ifndef NETWORKING_RECEIVER_H_
#define NETWORKING_RECEIVER_H_

#include <pthread.h>

#include "Tools/octetStream.h"
#include "Tools/WaitQueue.h"
#include "Tools/time-func.h"

class Receiver
{
    int socket;
    WaitQueue<octetStream*> in;
    WaitQueue<octetStream*> out;
    pthread_t thread;

    // prevent copying
    Receiver(const Receiver& other);

public:
    Timer timer;

    Receiver(int socket);

    void start();
    void stop();
    void run();

    void request(octetStream& os);
    void wait(octetStream& os);
};

#endif /* NETWORKING_RECEIVER_H_ */
