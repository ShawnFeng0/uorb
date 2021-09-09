//
// Copyright (c) 2021 shawnfeng. All rights reserved.
//
#pragma once

//---------------------------------------------------------
// Reference:
// https://github.com/preshing/cpp11-on-multicore/blob/master/common/sema.h
// Semaphore (POSIX, Linux)
//---------------------------------------------------------

#include <pthread.h>
#include <semaphore.h>

#include <cerrno>
#include <cstdint>

namespace uorb {
namespace base {

class Semaphore {
 public:
  explicit Semaphore(unsigned int count) { sem_init(&m_sema, 0, count); }
  Semaphore() = delete;
  Semaphore(const Semaphore &other) = delete;
  Semaphore &operator=(const Semaphore &other) = delete;

  ~Semaphore() { sem_destroy(&m_sema); }

  // increments the internal counter and unblocks acquirers
  void release() { sem_post(&m_sema); }

  // decrements the internal counter or blocks until it can
  void acquire() {
    // http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
#if defined(__unix__)
    while (-1 == sem_wait(&m_sema) && errno == EINTR) {
    }
#else
    sem_wait(&m_sema);
#endif
  }

  // tries to decrement the internal counter without blocking
  bool try_acquire() { return sem_trywait(&m_sema) == 0; }

  // tries to decrement the internal counter, blocking for up to a duration time
  bool try_acquire_for(int time_ms) {
    struct timespec abs_time {};
    GenerateFutureTime(CLOCK_REALTIME, time_ms, &abs_time);
    return try_acquire_until(abs_time);
  }

 private:
  sem_t m_sema{};

  // Hide this function in case the client is not sure which clockid to use
  // tries to decrement the internal counter, blocking until a point in time
  bool try_acquire_until(const struct timespec &abs_time) {
    return sem_timedwait(&m_sema, &abs_time) == 0;
  }

  // Increase time_ms time based on the current clockid time
  static inline void GenerateFutureTime(clockid_t clockid, uint32_t time_ms,
                                        struct timespec *out_ptr) {
    if (!out_ptr) return;
    auto &out = *out_ptr;
    // Calculate an absolute time in the future
    const decltype(out.tv_nsec) kSec2Nsec = 1000 * 1000 * 1000;
    clock_gettime(clockid, &out);
    uint64_t nano_secs = out.tv_nsec + ((uint64_t)time_ms * 1000 * 1000);
    out.tv_nsec = nano_secs % kSec2Nsec;
    out.tv_sec += nano_secs / kSec2Nsec;
  }
};

}  // namespace base
}  // namespace uorb
