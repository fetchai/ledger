#include "Waitable.hpp"

Notification::NotificationBuilder Waitable::makeNotification(void)
{
  Lock lock(mutex);
  auto n = Notification::create();
  waiting.push_back(n);
  return Notification::NotificationBuilder(n);
}

void Waitable::wake(void)
{
  Waiting waiting_local;
  {
    Lock lock(mutex);
    waiting_local.swap(waiting);
  }

  for(auto& waiter : waiting_local)
  {
    waiter -> Notify();
  }
}


void swap(Waitable& v1, Waitable& v2) {
  v1.swap(v2);
}

void Waitable::cancel(void)
{
  Waiting waiting_local;
  {
    Lock lock(mutex);
    cancelled = true;
    waiting_local.swap(waiting);
  }

  for(auto& waiter : waiting_local)
  {
    waiter -> Fail();
  }
}
