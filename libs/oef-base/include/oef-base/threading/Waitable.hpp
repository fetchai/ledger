#pragma once

#include "oef-base/threading/Notification.hpp"
#include <atomic>
#include <mutex>
#include <vector>

class Waitable
{
public:
  using Mutex   = std::mutex;
  using Lock    = std::lock_guard<Mutex>;
  using Waiting = std::vector<Notification::Notification>;

  Waitable()
    : cancelled{false}
    , woken_(false)
  {}
  virtual ~Waitable() = default;

  Notification::NotificationBuilder MakeNotification(void);
  void                              wake(void);

  void swap(Waitable &other)
  {
    Lock lock_other(other.mutex);
    Lock lock(mutex);
    std::swap(waiting, other.waiting);
    std::swap(cancelled, other.cancelled);
  }

  void cancel(void);

  bool IsCancelled(void) const
  {
    Lock lock(mutex);
    return cancelled;
  }

protected:
  Waiting           waiting;
  bool              cancelled;
  std::atomic<bool> woken_;
  mutable Mutex     mutex;

private:
  Waitable(const Waitable &other) = delete;              // { copy(other); }
  Waitable &operator=(const Waitable &other)  = delete;  // { copy(other); return *this; }
  bool      operator==(const Waitable &other) = delete;  // const { return compare(other)==0; }
  bool      operator<(const Waitable &other)  = delete;  // const { return compare(other)==-1; }
};

void swap(Waitable &v1, Waitable &v2);
