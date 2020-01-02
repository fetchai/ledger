#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "logging/logging.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <utility>

namespace fetch {
namespace oef {
namespace base {

namespace Notification {

class NotificationBuilder;

class NotificationImplementation
{
  friend class NotificationBuilder;

public:
  using Callback = std::function<void()>;

  enum class State
  {
    WAITING,
    SUCCESS,
    FAILED,
    TIMEDOUT
  };

  void Fail()
  {
    UpdateState(State::FAILED);
  }

  void Notify()
  {
    UpdateState(State::SUCCESS);
  }

  NotificationBuilder WithHandlers();

private:
  using Mutex       = std::mutex;
  using AtomicState = std::atomic<State>;
  using Condition   = std::condition_variable;

  mutable Mutex     notify_lock_;
  mutable Condition notify_;
  AtomicState       state_{State::WAITING};

  bool IsWaiting() const
  {
    return (State::WAITING == state_);
  }

  void UpdateState(State state);
  void DispatchCallbacks();

  void SetSuccessCallback(Callback const &cb)
  {
    callback_success_ = cb;
  }
  void SetFailureCallback(Callback const &cb)
  {
    callback_failure_ = cb;
  }
  void SetCompletionCallback(Callback const &cb)
  {
    callback_complete_ = cb;
  }

  Callback callback_success_;
  Callback callback_failure_;
  Callback callback_complete_;
};

using Notification = std::shared_ptr<NotificationImplementation>;

class NotificationBuilder
{
public:
  using Callback = NotificationImplementation::Callback;

  explicit NotificationBuilder(Notification notification, bool execute_now = false)
    : notification_(std::move(notification))
    , execute_now_{execute_now}
  {}

  explicit NotificationBuilder() = default;

  ~NotificationBuilder()
  {
    try
    {
      if (notification_)
      {
        notification_->SetSuccessCallback(callback_success_);
        notification_->SetFailureCallback(callback_failure_);
        notification_->SetCompletionCallback(callback_complete_);

        // in the rare (probably failure case) when the notification has been resolved during
        // before the responses have been set
        if (execute_now_)
        {
          notification_->Notify();
        }

        if (!notification_->IsWaiting())
        {
          notification_->DispatchCallbacks();
        }
      }
    }
    catch (...)
    {
    }
  }

  NotificationBuilder &Then(Callback const &cb)
  {
    callback_success_ = cb;
    return *this;
  }

  NotificationBuilder &Catch(Callback const &cb)
  {
    callback_failure_ = cb;
    return *this;
  }

  NotificationBuilder &Finally(Callback const &cb)
  {
    callback_complete_ = cb;
    return *this;
  }

  NotificationBuilder &Cancel()
  {
    notification_.reset();
    return *this;
  }

  bool Waiting()
  {
    return notification_ && notification_->IsWaiting();
  }

  NotificationBuilder(NotificationBuilder &&other)
    : notification_(other.notification_)
    , callback_success_(other.callback_success_)
    , callback_failure_(other.callback_failure_)
    , callback_complete_(other.callback_complete_)
  {}

private:
  Notification notification_;
  bool         execute_now_;

  Callback callback_success_;
  Callback callback_failure_;
  Callback callback_complete_;
};

Notification        create();
NotificationBuilder builder(Notification n);

}  // namespace Notification
}  // namespace base
}  // namespace oef
}  // namespace fetch
