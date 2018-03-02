// (C) 2018 University of Bristol. See License.txt

/*
 * Lock.cpp
 *
 */

#include <Tools/Lock.h>

Lock::Lock()
{
    pthread_mutex_init(&mutex, 0);
}

Lock::~Lock()
{
    pthread_mutex_destroy(&mutex);
}

void Lock::lock()
{
    pthread_mutex_lock(&mutex);
}

void Lock::unlock()
{
    pthread_mutex_unlock(&mutex);
}
