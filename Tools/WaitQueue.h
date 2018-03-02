// (C) 2018 University of Bristol. See License.txt

/*
 * WaitQueue.h
 *
 */

#ifndef TOOLS_WAITQUEUE_H_
#define TOOLS_WAITQUEUE_H_

#include <pthread.h>
#include <deque>
using namespace std;

template<class T>
class WaitQueue
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    deque<T> queue;
    bool running;

    // prevent copying
    WaitQueue(const WaitQueue& other);

public:
    WaitQueue() : running(true)
    {
        pthread_mutex_init(&mutex, 0);
        pthread_cond_init(&cond, 0);
    }

    ~WaitQueue()
    {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&mutex);
    }

    void wait()
    {
        pthread_cond_wait(&cond, &mutex);
    }

    void signal()
    {
        pthread_cond_signal(&cond);
    }

    void push(const T& value)
    {
        lock();
        queue.push_back(value);
        signal();
        unlock();
    }

    bool pop(T& value)
    {
        lock();
        if (running and queue.size() == 0)
            wait();
        if (running)
        {
            value = queue.front();
            queue.pop_front();
        }
        unlock();
        return running;
    }

    void stop()
    {
        lock();
        running = false;
        signal();
        unlock();
    }
};

#endif /* TOOLS_WAITQUEUE_H_ */
