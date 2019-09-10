#include "Notification.hpp"

#include <cassert>

void Notification::NotificationImplementation::DispatchCallbacks()
{
  Callback *handler = nullptr;

  switch (state_.load())
  {
  case State::SUCCESS:
    handler = &callback_success_;
    break;
  case State::FAILED:
    handler = &callback_failure_;
    break;
  default:
    break;
  }

  // dispatch the event
  if (handler && *handler)
  {
    // call the success or failure handler
    (*handler)();
  }

  // call the finally handler
  if (callback_complete_)
  {
    callback_complete_();
  }

  // invalidate the callbacks
  callback_success_    = nullptr;
  callback_failure_    = nullptr;
  callback_complete_   = nullptr;
}

void Notification::NotificationImplementation::UpdateState(State state)
{
  assert(state != State::WAITING);

  bool dispatch = false;

  {
    std::unique_lock<std::mutex> lock(notify_lock_);
    if (state_ == State::WAITING)
    {
      state_   = state;
      dispatch = true;
    }
  }

  if (dispatch)
  {
    // wake up all the pending threads
    notify_.notify_all();
    DispatchCallbacks();
  }
}

Notification::Notification Notification::create()
{
  return std::make_shared<NotificationImplementation>();
}
