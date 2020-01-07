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

#include "core/serializers/exception.hpp"
#include "network/service/promise.hpp"

#include <chrono>

namespace fetch {
namespace service {
namespace details {
namespace {

using byte_array::ConstByteArray;
using serializers::SerializableException;

using Counter = PromiseImplementation::Counter;
using State   = PromiseImplementation::State;

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
Mutex                          PromiseImplementation::counter_lock_;

std::chrono::seconds const PromiseImplementation::DEFAULT_TIMEOUT{30};

PromiseImplementation::PromiseImplementation(uint64_t protocol, uint64_t function)
  : protocol_{protocol}
  , function_{function}
{}

ConstByteArray const &PromiseImplementation::value() const
{
  return value_;
}

Counter PromiseImplementation::id() const
{
  return id_;
}

PromiseImplementation::Timepoint const &PromiseImplementation::created_at() const
{
  return created_;
}

PromiseImplementation::Timepoint const &PromiseImplementation::deadline() const
{
  return deadline_;
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

std::string const &PromiseImplementation::name() const
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

void PromiseImplementation::Timeout()
{
  UpdateState(State::TIMEDOUT);
}

void PromiseImplementation::Fail()
{
  UpdateState(State::FAILED);
}

bool PromiseImplementation::Wait(bool throw_exception, uint64_t extend_wait_by) const
{
  auto recalculated_deadline = deadline_ + std::chrono::seconds{extend_wait_by};

  if (Clock::now() >= recalculated_deadline)
  {
    LogTimout(name_, id_);
    return false;
  }

  std::unique_lock<std::mutex> lock(notify_lock_);
  while (State::WAITING == state())
  {
    if (std::cv_status::timeout == notify_.wait_until(lock, recalculated_deadline))
    {
      LogTimout(name_, id_);
      return false;
    }
  }
  State state_copy = state();

  if (State::FAILED == state_copy)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Promise ", id_, " failed!");

    if (throw_exception && exception_)
    {
      throw SerializableException{*exception_};
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
    FETCH_LOCK(notify_lock_);
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
  if ((handler != nullptr) && *handler)
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

// promise builder

PromiseBuilder::PromiseBuilder(PromiseImplementation &promise)
  : promise_(promise)
{}

PromiseBuilder::~PromiseBuilder()
{
  promise_.SetSuccessCallback(callback_success_);
  promise_.SetFailureCallback(callback_failure_);
  promise_.SetCompletionCallback(callback_complete_);

  // in the rare (probably failure case) when the promise has been resolved during before the
  // responses have been set
  if (!promise_.IsWaiting())
  {
    promise_.DispatchCallbacks();
  }
}

PromiseBuilder &PromiseBuilder::Then(Callback const &cb)
{
  callback_success_ = cb;
  return *this;
}

PromiseBuilder &PromiseBuilder::Catch(Callback const &cb)
{
  callback_failure_ = cb;
  return *this;
}

PromiseBuilder &PromiseBuilder::Finally(Callback const &cb)
{
  callback_complete_ = cb;
  return *this;
}

}  // namespace details

PromiseError::PromiseError(details::PromiseImplementation const &promise)
  : id_{promise.id()}
  , created_{promise.created_at()}
  , deadline_{promise.deadline()}
  , protocol_{promise.protocol()}
  , function_{promise.function()}
  , state_{promise.state()}
  , name_{promise.name()}
  , message_{BuildErrorMessage(*this)}
{}

PromiseError::PromiseError(PromiseError const &other) noexcept
  : id_{other.id_}
  , created_{other.created_}
  , deadline_{other.deadline_}
  , protocol_{other.protocol_}
  , function_{other.function_}
  , state_{other.state_}
  , name_{other.name_}
  , message_{other.message_}
{}

char const *PromiseError::what() const noexcept
{
  return message_.c_str();
}

std::string PromiseError::BuildErrorMessage(PromiseError const &error)
{
  std::ostringstream oss;

  oss << "Promise ";
  switch (error.state_)
  {
  case details::PromiseImplementation::State::TIMEDOUT:
    oss << "timeout";
    break;
  case details::PromiseImplementation::State::FAILED:
    oss << "failure";
    break;
  default:
    oss << "error";
    break;
  }

  if (error.protocol_ != details::PromiseImplementation::INVALID)
  {
    oss << " Protocol: " << error.protocol_;
  }

  if (error.function_ != details::PromiseImplementation::INVALID)
  {
    oss << " Function: " << error.function_;
  }

  if (!error.name_.empty())
  {
    oss << " Name: " << error.name_;
  }

  return oss.str();
}

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

Promise MakePromise()
{
  return std::make_shared<details::PromiseImplementation>();
}

Promise MakePromise(uint64_t pro, uint64_t func)
{
  return std::make_shared<details::PromiseImplementation>(pro, func);
}

}  // namespace service
}  // namespace fetch
