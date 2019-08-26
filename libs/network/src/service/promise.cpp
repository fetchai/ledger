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

using byte_array::ConstByteArray;
using serializers::SerializableException;

using Counter = PromiseImplementation::Counter;
using State = PromiseImplementation::State;

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
PromiseImplementation::Mutex   PromiseImplementation::PromiseImplementation::counter_lock_{__LINE__, __FILE__};

std::chrono::seconds const PromiseImplementation::DEFAULT_TIMEOUT{30};

PromiseImplementation::PromiseImplementation(uint64_t pro, uint64_t func)
  : protocol_{pro}
  , function_{func}
{
}

ConstByteArray const &PromiseImplementation::value() const
{
  return value_;
}

Counter PromiseImplementation::id() const
{
  return id_;
}

uint64_t PromiseImplementation::protocol() const
{
  return protocol_;
}

uint64_t PromiseImplementation::function() const
{
  return function_;
}

State PromiseImplementation::state() const
{
  if (Clock::now() >= deadline_)
  {
    UpdateState(State::TIMEDOUT);
  }

  return state_;
}

SerializableException const &PromiseImplementation::exception() const
{
  return (*exception_);
}

const std::string &PromiseImplementation::name() const
{
  return name_;
}

bool PromiseImplementation::IsWaiting() const
{
  return (State::WAITING == state());
}

bool PromiseImplementation::IsSuccessful() const
{
  return (State::SUCCESS == state());
}

bool PromiseImplementation::IsFailed() const
{
  return (State::FAILED == state());
}

PromiseBuilder PromiseImplementation::WithHandlers()
{
  return PromiseBuilder{*this};
}

void PromiseImplementation::Fulfill(ConstByteArray const &value)
{
  value_ = value;

  UpdateState(State::SUCCESS);
}

void PromiseImplementation::Fail(SerializableException const &exception)
{
  exception_ = std::make_unique<SerializableException>(exception);

  UpdateState(State::FAILED);
}

void PromiseImplementation::Fail()
{
  UpdateState(State::FAILED);
}

bool PromiseImplementation::Wait(bool throw_exception) const
{
  State state_copy{state()};

  if (Clock::now() >= deadline_)
  {
    LogTimout(name_, id_);
    return false;
  }
  else
  {
    std::unique_lock<std::mutex> lock(notify_lock_);
    while (State::WAITING == state())
    {
      if (std::cv_status::timeout == notify_.wait_until(lock, deadline_))
      {
        LogTimout(name_, id_);
        return false;
      }
    }
    state_copy = state();
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

  return true;
}

void PromiseImplementation::SetSuccessCallback(Callback const &cb)
{
  FETCH_LOCK(callback_lock_);
  callback_success_ = cb;
}

void PromiseImplementation::SetFailureCallback(Callback const &cb)
{
  FETCH_LOCK(callback_lock_);
  callback_failure_ = cb;
}

void PromiseImplementation::SetCompletionCallback(Callback const &cb)
{
  FETCH_LOCK(callback_lock_);
  callback_completion_ = cb;
}

void PromiseImplementation::UpdateState(State state) const
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

void PromiseImplementation::DispatchCallbacks() const
{
  FETCH_LOCK(callback_lock_);

  Callback const *handler = nullptr;

  switch (state())
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
  callback_success_    = nullptr;
  callback_failure_    = nullptr;
  callback_completion_ = nullptr;
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
