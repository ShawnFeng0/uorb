//
// Created by shawnfeng on 2021/10/8.
//

#include <sys/epoll.h>
#include <uorb/internal/noncopyable.h>

#include <vector>

#include "subscription_impl.h"

namespace uorb {

class EventPollImpl : private internal::Noncopyable {
 public:
  void Add(SubscriptionImpl &sub) {
    subs.insert(&sub);
    sub.RegisterCallback(&semaphore_callback_);
  }

  void Delete(SubscriptionImpl &sub) {
    sub.UnregisterCallback(&semaphore_callback_);
    subs.erase(&sub);
  }

  int Wait(int timeout_ms) {
    return semaphore_callback_.try_acquire_for(timeout_ms);
  }

  ~EventPollImpl() {
    for (auto &sub : subs) sub->UnregisterCallback(&semaphore_callback_);
  }

 private:
  std::set<SubscriptionImpl *> subs;
  uorb::SemaphoreCallback semaphore_callback_;
};

}  // namespace uorb
