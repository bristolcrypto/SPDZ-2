// (C) 2018 University of Bristol. See License.txt

/*
 * Lock.h
 *
 */

#ifndef TOOLS_LOCK_H_
#define TOOLS_LOCK_H_

#include <pthread.h>

class Lock
{
    pthread_mutex_t mutex;
public:
    Lock();
    virtual ~Lock();

    void lock();
    void unlock();
};

#endif /* TOOLS_LOCK_H_ */
