//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "network/service/promise.hpp"

namespace fetch {
namespace service {
namespace details {

namespace {
template <typename NAME, typename ID>
void LogTimout(NAME const &name, ID const &id)
{
  if (name.length() > 0)
  {
    FETCH_LOG_WARN(PromiseImplementation::LOGGING_NAME, "Promise '", name, "'timed out!");
  }
  else
  {
    FETCH_LOG_WARN(PromiseImplementation::LOGGING_NAME, "Promise ", id, " timed out!");
  }
}
}  // namespace

PromiseImplementation::Counter PromiseImplementation::counter_{0};
PromiseImplementation::Mutex   PromiseImplementation::counter_lock_{__LINE__, __FILE__};

PromiseBuilder PromiseImplementation::WithHandlers()
{
  return PromiseBuilder{*this};
}

//#undef FETCH_PROMISE_CV
/**
 * Wait for the promise to conclude.
 *
 * @param timeout_ms The timeout (in milliseconds) to wait for the promise to be available
 * @param throw_exception Signal if the promise has failed, if it should throw the associated
 * exception
 * @return true if the promise was fulfilled, otherwise false
 **/
bool PromiseImplementation::Wait(uint32_t timeout_ms, bool throw_exception) const
{
  LOG_STACK_TRACE_POINT;
  bool const has_timeout = (timeout_ms > 0) && (timeout_ms < FOREVER);

#ifndef FETCH_PROMISE_CV
  if (has_timeout)
  {
    Timepoint const timeout_deadline = Clock::now() + std::chrono::milliseconds{timeout_ms};
    while (timeout_ms && IsWaiting())
    {
      // determine if timeout has expired
      if (Clock::now() >= timeout_deadline)
      {
        LogTimout(name_, id_);
        return false;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  if (IsWaiting())
  {
    return false;
  }
  else if (IsFailed())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Promise ", id_, " failed!");

    if (throw_exception && exception_)
    {
      throw *exception_;
    }

    return false;
  }
#else   // FETCH_PROMISE_CV
  State state_copy{state_};
  if (has_timeout)
  {
    Timepoint const timeout_deadline = Clock::now() + std::chrono::milliseconds{timeout_ms};
    FETCH_LOCK(notify_lock_);
    while (State::WAITING == state_)
    {
      if (std::cv_status::timeout == notify_.wait_until(lock, timeout_deadline))
      {
        LogTimout(name_, id_);
        return false;
      }
    }
    state_copy = state_;
  }

  if (State::FAILED == state_copy)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Promise ", id_, " failed!");

    if (throw_exception && exception_)
    {
      throw *exception_;
    }

    return false;
  }
#endif  // FETCH_PROMISE_CV

  return true;
}

void PromiseImplementation::UpdateState(State state)
{
  LOG_STACK_TRACE_POINT;

#ifdef FETCH_PROMISE_CV

  bool dispatch = false;

  {
    FETCH_LOCK(notify_lock_);
    if (state_ == State::WAITING)
    {
      state_   = state;
      dispatch = true;

      // wake up all the pending threads
      notify_.notify_all();
    }
  }

  if (dispatch)
  {
    DispatchCallbacks();
  }

#else
  if (state_.exchange(state) == State::WAITING)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Promise ", id_, " set from Waiting to ", ToString(state));

    DispatchCallbacks();
  }
#endif
}

void PromiseImplementation::DispatchCallbacks()
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
  if (callback_completion_)
  {
    callback_completion_();
  }

  // invalidate the callbacks
  callback_success_    = Callback{};
  callback_failure_    = Callback{};
  callback_completion_ = Callback{};
}

PromiseImplementation::Counter PromiseImplementation::GetNextId()
{
  FETCH_LOCK(counter_lock_);
  return counter_++;
}

}  // namespace details

/**
 * Converts the state of the promise to a string
 *
 * @param state The specified state to translate
 * @return
 */
char const *ToString(PromiseState state)
{
  char const *name = "Unknown";
  switch (state)
  {
  case details::PromiseImplementation::State::TIMEDOUT:
    name = "Timedout";
    break;
  case details::PromiseImplementation::State::WAITING:
    name = "Waiting";
    break;
  case details::PromiseImplementation::State::SUCCESS:
    name = "Success";
    break;
  case details::PromiseImplementation::State::FAILED:
    name = "Failed";
    break;
  }

  return name;
}

static const std::array<PromiseState, 4> promise_states{
    {PromiseState::WAITING, PromiseState::SUCCESS, PromiseState::FAILED, PromiseState::TIMEDOUT}};

const std::array<PromiseState, 4> &GetAllPromiseStates()
{
  return promise_states;
}

}  // namespace service
}  // namespace fetch
