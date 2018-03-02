// (C) 2018 University of Bristol. See License.txt

/*
 * Sender.cpp
 *
 */

#include "Sender.h"

void* run_sender_thread(void* sender)
{
    ((Sender*)sender)->run();
    return 0;
}

Sender::Sender(int socket) : socket(socket), thread(0)
{
}

void Sender::start()
{
    pthread_create(&thread, 0, run_sender_thread, this);
}

void Sender::stop()
{
    in.stop();
    pthread_join(thread, 0);
}

void Sender::run()
{
    const octetStream* os = 0;
    while (in.pop(os))
    {
//        timer.start();
        os->Send(socket);
//        timer.stop();
        out.push(os);
    }
}

void Sender::request(const octetStream& os)
{
    in.push(&os);
}

void Sender::wait(const octetStream& os)
{
    const octetStream* queued = 0;
    out.pop(queued);
    if (queued != &os)
      throw not_implemented();
}
