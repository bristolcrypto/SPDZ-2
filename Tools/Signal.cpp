// (C) 2017 University of Bristol. See License.txt

/*
 * Signal.cpp
 *
 */

#include "Signal.h"

Signal::Signal()
{
    pthread_mutex_init(&mutex, 0);
    pthread_cond_init(&cond, 0);
}

Signal::~Signal()
{
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
}

void Signal::lock()
{
    pthread_mutex_lock(&mutex);
}

void Signal::unlock()
{
    pthread_mutex_unlock(&mutex);
}

void Signal::wait()
{
    pthread_cond_wait(&cond, &mutex);
}

void Signal::broadcast()
{
    pthread_cond_broadcast(&cond);
}
