// (C) 2018 University of Bristol. See License.txt

/*
 * Receiver.cpp
 *
 */

#include "Receiver.h"

#include <iostream>
using namespace std;

void* run_receiver_thread(void* receiver)
{
    ((Receiver*)receiver)->run();
    return 0;
}

Receiver::Receiver(int socket) : socket(socket), thread(0)
{
}

void Receiver::start()
{
    pthread_create(&thread, 0, run_receiver_thread, this);
}

void Receiver::stop()
{
    in.stop();
    pthread_join(thread, 0);
}

void Receiver::run()
{
    octetStream* os = 0;
    while (in.pop(os))
    {
        os->reset_write_head();
        timer.start();
        os->Receive(socket);
        timer.stop();
        out.push(os);
    }
}

void Receiver::request(octetStream& os)
{
    in.push(&os);
}

void Receiver::wait(octetStream& os)
{
    octetStream* queued = 0;
    out.pop(queued);
    if (queued != &os)
      throw not_implemented();
}
