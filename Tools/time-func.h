// (C) 2018 University of Bristol. See License.txt

#ifndef _timer
#define _timer

#include <sys/wait.h>   /* Wait for Process Termination */
#include <sys/time.h>
#include <time.h>

#include "Exceptions/Exceptions.h"

long long timeval_diff(struct timeval *start_time, struct timeval *end_time);
double timeval_diff_in_seconds(struct timeval *start_time, struct timeval *end_time);
long long timespec_diff(struct timespec *start_time, struct timespec *end_time);

class Timer
{
  public:
  Timer(clockid_t clock_id = CLOCK_MONOTONIC) : running(false), elapsed_time(0), clock_id(clock_id)
      { clock_gettime(clock_id, &startv); }
  Timer& start();
  void stop();
  void reset();

  double elapsed();
  double idle();

  private:
  timespec startv;
  bool running;
  long long elapsed_time;
  clockid_t clock_id;

  long long elapsed_since_last_start();
};

class TimeScope
{
  Timer& timer;

public:
  TimeScope(Timer& timer) : timer(timer) { timer.start(); }
  ~TimeScope() { timer.stop(); }
};

inline Timer& Timer::start()
{
  if (running)
    throw Processor_Error("Timer already running.");
  // clock() is not suitable in threaded programs so time using something else
  clock_gettime(clock_id, &startv);
  running = true;
  return *this;
}

inline void Timer::stop()
{
  if (!running)
    throw Processor_Error("Time not running.");
  elapsed_time += elapsed_since_last_start();

  running = false;
  clock_gettime(clock_id, &startv);
}

inline void Timer::reset()
{
  elapsed_time = 0;
  clock_gettime(clock_id, &startv);
}

inline long long Timer::elapsed_since_last_start()
{
  timespec endv;
  clock_gettime(clock_id, &endv);
  return timespec_diff(&startv, &endv);
}

#endif

