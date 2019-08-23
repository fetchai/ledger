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
Mutex                          PromiseImplementation::counter_lock_{__LINE__, __FILE__};

PromiseBuilder PromiseImplementation::WithHandlers()
{
  return PromiseBuilder{*this};
}

/**
 * Wait for the promise to conclude.
 *
 * @param timeout_ms The timeout (in milliseconds) to wait for the promise to be available
 * @param throw_exception Signal if the promise has failed, if it should throw the associated
 * exception
 * @return true if the promise was fulfilled, otherwise false
 */
bool PromiseImplementation::Wait(uint32_t timeout_ms, bool throw_exception) const
{
  bool const has_timeout = timeout_ms > 0;
  State      state_copy{state_};
  if (has_timeout)
  {
    Timepoint const timeout_deadline = Clock::now() + std::chrono::milliseconds{timeout_ms};
    std::unique_lock<std::mutex> lock(notify_lock_);
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

  return true;
}

void PromiseImplementation::UpdateState(State state)
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

void PromiseImplementation::DispatchCallbacks()
{
  FETCH_LOCK(callback_lock_);

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
  callback_success_    = nullptr;
  callback_failure_    = nullptr;
  callback_completion_ = nullptr;
}

PromiseImplementation::Counter PromiseImplementation::GetNextId()
{
  FETCH_LOCK(counter_lock_);
  return counter_++;
}

PromiseImplementation::PromiseImplementation(uint64_t pro, uint64_t func)
  : protocol_(pro)
  , function_(func)
{}

PromiseImplementation::ConstByteArray const &PromiseImplementation::value() const
{
  return value_;
}

PromiseImplementation::Counter PromiseImplementation::id() const
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

PromiseImplementation::State PromiseImplementation::state() const
{
  return state_;
}

PromiseImplementation::SerializableException const &PromiseImplementation::exception() const
{
  return (*exception_);
}

bool PromiseImplementation::IsWaiting() const
{
  return (State::WAITING == state_);
}

bool PromiseImplementation::IsSuccessful() const
{
  return (State::SUCCESS == state_);
}

bool PromiseImplementation::IsFailed() const
{
  return (State::FAILED == state_);
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

std::string &PromiseImplementation::name()
{
  return name_;
}

const std::string &PromiseImplementation::name() const
{
  return name_;
}

PromiseImplementation::State PromiseImplementation::GetState() const
{
  return state_;
}

bool PromiseImplementation::Wait(bool throw_exception) const
{
  return Wait(FOREVER, throw_exception);
}

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
