#pragma once

#include <pthread.h>

#include "orb_errno.h"
#include "orb_mutex.hpp"

namespace uORB {

template <int clock_id>
class ConditionVariable {
  pthread_cond_t cond_{};

public:
  ConditionVariable() noexcept {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    pthread_condattr_setclock(&attr, (clock_id));
    pthread_cond_init((&cond_), &attr);
  }

  ~ConditionVariable() noexcept { pthread_cond_destroy(&cond_); }

 public:
  ConditionVariable(const ConditionVariable &) = delete;
  ConditionVariable &operator=(const ConditionVariable &) = delete;

  int notify_one() noexcept { return pthread_cond_signal(&cond_); }

  int notify_all() noexcept { return pthread_cond_broadcast(&cond_); }

  int wait(Mutex &lock) noexcept {
    return pthread_cond_wait(&cond_, lock.native_handle());
  }

  template <typename _Predicate>
  int wait(Mutex &lock, _Predicate p) {
    int ret = 0;
    while (!p()) ret = wait(lock);
    return ret;
  }

  int wait_until(Mutex &lock, const struct timespec &atime) {
    return pthread_cond_timedwait(&cond_, lock.native_handle(), &atime);
  }

  int wait_until(Mutex &lock, const struct timespec *atime) {
    return pthread_cond_timedwait(&cond_, lock.native_handle(), atime);
  }

  template <typename _Predicate>
  bool wait_until(Mutex &lock, const struct timespec &atime, _Predicate p) {
    while (!p())
      if (wait_until(lock, atime) == ETIMEDOUT) return p();
    return true;
  }

  int wait_for(Mutex &lock, long usec) {
    struct timespec req {};
    clock_gettime(clock_id, &req);
    req.tv_nsec += usec * 1000;
    req.tv_sec += req.tv_nsec / (1e9);
    return wait_until(lock, req);
  }

  pthread_cond_t *native_handle() { return &cond_; }
};

typedef ConditionVariable<CLOCK_MONOTONIC> MonoClockCond;
typedef ConditionVariable<CLOCK_REALTIME> RealClockCond;

}  // namespace uORB
