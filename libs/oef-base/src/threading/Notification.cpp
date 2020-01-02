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

#include "oef-base/threading/Notification.hpp"

#include <cassert>

namespace fetch {
namespace oef {
namespace base {

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
  if ((handler == nullptr) && *handler)
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
  callback_success_  = nullptr;
  callback_failure_  = nullptr;
  callback_complete_ = nullptr;
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

}  // namespace base
}  // namespace oef
}  // namespace fetch
