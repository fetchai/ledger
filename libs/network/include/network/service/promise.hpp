#pragma once
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

#include "core/byte_array/byte_array.hpp"
#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "core/serializers/exception.hpp"
#include "network/service/types.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace fetch {
namespace service {

namespace details {

class PromiseBuilder;

class PromiseImplementation
{
  friend PromiseBuilder;

public:
  PromiseImplementation(uint64_t pro, uint64_t func)
  {
    this->protocol_ = pro;
    this->function_ = func;
  }

  PromiseImplementation() = default;

  using Counter               = uint64_t;
  using ConstByteArray        = byte_array::ConstByteArray;
  using SerializableException = serializers::SerializableException;
  using ExceptionPtr          = std::unique_ptr<SerializableException>;
  using Callback              = std::function<void()>;
  using Clock                 = std::chrono::high_resolution_clock;
  using Timepoint             = Clock::time_point;

  static constexpr char const *LOGGING_NAME = "Promise";
  static constexpr uint32_t    FOREVER      = std::numeric_limits<uint32_t>::max();

  enum class State
  {
    WAITING,
    SUCCESS,
    FAILED,
    TIMEDOUT
  };

#if 0
  static char const *ToString(State state)
  {
    switch (state)
    {
    case State::WAITING:
      return "Waiting";
    case State::SUCCESS:
      return "Success";
    case State::FAILED:
      return "Failed";
    case State::TIMEDOUT:
      return "Timeout";
    default:
      return "Unknown";
    }
  }
#endif

  ConstByteArray const &value() const
  {
    return value_;
  }
  Counter id() const
  {
    return id_;
  }
  uint64_t protocol() const
  {
    return protocol_;
  }
  uint64_t function() const
  {
    return function_;
  }
  State state() const
  {
    return state_;
  }
  SerializableException const &exception() const
  {
    return (*exception_);
  }

  /// @name State Access
  /// @{
  bool IsWaiting() const
  {
    return (State::WAITING == state_);
  }
  bool IsSuccessful() const
  {
    return (State::SUCCESS == state_);
  }
  bool IsFailed() const
  {
    return (State::FAILED == state_);
  }
  /// @}
private:
  /// @name Callback Handlers
  /// @{
  void SetSuccessCallback(Callback const &cb)
  {
    FETCH_LOCK(callback_lock_);
    callback_success_ = cb;
  }

  void SetFailureCallback(Callback const &cb)
  {
    FETCH_LOCK(callback_lock_);
    callback_failure_ = cb;
  }

  void SetCompletionCallback(Callback const &cb)
  {
    FETCH_LOCK(callback_lock_);
    callback_completion_ = cb;
  }
  /// @}

  uint64_t protocol_ = uint64_t(-1);
  uint64_t function_ = uint64_t(-1);

public:
  PromiseBuilder WithHandlers();

  /// @name Promise Results
  /// @{
  void Fulfill(ConstByteArray const &value)
  {
    LOG_STACK_TRACE_POINT;

    value_ = value;

    UpdateState(State::SUCCESS);
  }

  void Fail(SerializableException const &exception)
  {
    LOG_STACK_TRACE_POINT;

    exception_ = std::make_unique<SerializableException>(exception);

    UpdateState(State::FAILED);
  }

  void Fail()
  {
    UpdateState(State::FAILED);
  }
  /// @}

  std::string &name()
  {
    return name_;
  }
  const std::string &name() const
  {
    return name_;
  }

  State GetState() const
  {
    return state_;
  }

  /// @name Waits
  /// @{
  bool Wait(uint32_t timeout_ms = FOREVER, bool throw_exception = true) const;
  bool Wait(bool throw_exception) const
  {
    return Wait(FOREVER, throw_exception);
  }
  /// @}

  /// @name Result Access
  /// @{
  template <typename T>
  T As() const
  {
    LOG_STACK_TRACE_POINT;

    T result{};
    if (!As<T>(result))
    {
      throw std::runtime_error("Timeout or connection lost");
    }

    return result;
  }

  template <typename T>
  bool As(T &ret) const
  {
    LOG_STACK_TRACE_POINT;

    if (!Wait())
    {
      return false;
    }

    serializer_type ser(value_);
    ser >> ret;

    return true;
  }
  /// @}

private:
  using Mutex       = mutex::Mutex;
  using AtomicState = std::atomic<State>;
  using Condition   = std::condition_variable;

  void UpdateState(State state);
  void DispatchCallbacks();

  static Counter counter_;
  static Mutex   counter_lock_;
  static Counter GetNextId();

  Counter const  id_{GetNextId()};
  AtomicState    state_{State::WAITING};
  ConstByteArray value_;
  ExceptionPtr   exception_;
  std::string    name_;

  mutable Mutex  callback_lock_{__LINE__, __FILE__};
  Callback       callback_success_;
  Callback       callback_failure_;
  Callback       callback_completion_;


#define FETCH_PROMISE_CV
#ifdef FETCH_PROMISE_CV
  mutable Mutex     notify_lock_{__LINE__, __FILE__};
  mutable Condition notify_;
#endif
};

class PromiseBuilder
{
public:
  using Callback = PromiseImplementation::Callback;

  explicit PromiseBuilder(PromiseImplementation &promise)
    : promise_(promise)
  {}

  ~PromiseBuilder()
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

  PromiseBuilder &Then(Callback const &cb)
  {
    callback_success_ = cb;
    return *this;
  }

  PromiseBuilder &Catch(Callback const &cb)
  {
    callback_failure_ = cb;
    return *this;
  }

  PromiseBuilder &Finally(Callback const &cb)
  {
    callback_complete_ = cb;
    return *this;
  }

private:
  PromiseImplementation &promise_;

  Callback callback_success_;
  Callback callback_failure_;
  Callback callback_complete_;
};

}  // namespace details

using PromiseCounter = details::PromiseImplementation::Counter;
using PromiseState   = details::PromiseImplementation::State;
using Promise        = std::shared_ptr<details::PromiseImplementation>;
using PromiseStates  = std::array<PromiseState, 4>;

inline Promise MakePromise()
{
  return std::make_shared<details::PromiseImplementation>();
}

inline Promise MakePromise(uint64_t pro, uint64_t func)
{
  return std::make_shared<details::PromiseImplementation>(pro, func);
}

char const *         ToString(PromiseState state);
const PromiseStates &GetAllPromiseStates();

}  // namespace service
}  // namespace fetch
