#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <atomic>
#include <chrono>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <stdexcept>

namespace fetch {
namespace service {

#if 1
namespace details {

class PromiseBuilder;

class PromiseImplementation
{
  friend PromiseBuilder;

public:

  using Counter               = uint64_t;
  using ConstByteArray        = byte_array::ConstByteArray;
  using SerializableException = serializers::SerializableException;
  using ExceptionPtr          = std::unique_ptr<SerializableException>;
  using Callback = std::function<void()>;
  using Clock = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;

  static constexpr char const *LOGGING_NAME = "Promise";
  static constexpr uint32_t FOREVER = std::numeric_limits<uint32_t>::max();

  enum class State
  {
    WAITING,
    SUCCESS,
    FAILED
  };

  ConstByteArray const &value() const { return value_; }
  Counter id() const { return id_; }
  State state() const { return state_; }
  SerializableException const &exception() const { return (*exception_); }

  /// @name State Access
  /// @{
  bool IsWaiting() const { return (State::WAITING == state_); }
  bool IsSuccessful() const { return (State::SUCCESS == state_); }
  bool IsFailed() const { return (State::FAILED == state_); }
  /// @}

  /// @name Callback Handlers
  /// @{
  void SetSuccessCallback(Callback const &cb) { callback_success_ = cb; }
  void SetFailureCallback(Callback const &cb) { callback_failure_ = cb; }
  void SetCompletionCallback(Callback const &cb) { callback_completion_ = cb; }
  /// @}

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

#if 0
  Status WaitLoop(int milliseconds, int cycles)
  {
    auto s = GetStatus();
    while (cycles > 0 && s == WAITING)
    {
      cycles -= 1;
      s = GetStatus();
      if (s != WAITING) return s;

      FETCH_LOG_PROMISE();
      Wait(milliseconds, false);
      s = GetStatus();
      if (s != WAITING) return s;
    }
    return WAITING;
  }
#endif

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

  using Mutex = mutex::Mutex;
  using AtomicState = std::atomic<State>;

  void UpdateState(State state);
  void DispatchCallbacks();

  static Counter counter_;
  static Mutex counter_lock_;
  static Counter GetNextId();

  Counter const   id_{GetNextId()};
  AtomicState     state_{State::WAITING};
  ConstByteArray  value_;
  ExceptionPtr    exception_;
  Callback        callback_success_;
  Callback        callback_failure_;
  Callback        callback_completion_;
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

} // namespace details

using PromiseCounter = details::PromiseImplementation::Counter;
using PromiseState = details::PromiseImplementation::State;
using Promise = std::shared_ptr<details::PromiseImplementation>;

inline Promise MakePromise()
{
  return std::make_shared<details::PromiseImplementation>();
}

char const *ToString(PromiseState state);

#else


namespace details {

class PromiseImplementation
{
public:
  using promise_counter_type = uint64_t;
  using byte_array_type      = byte_array::ConstByteArray;
  typedef std::function<void(void)> callback_type;

  static constexpr char const *LOGGING_NAME = "PromiseImpl";

  typedef enum
  {
    NONE,
    SUCCESS,
    FAIL
  } Conclusion;

  PromiseImplementation()
  {
    LOG_STACK_TRACE_POINT;

    id_ = next_promise_id();
  }

  void ConcludeSuccess()
  {
    if (conclusion_.exchange(SUCCESS) == NONE)
    {
      auto func = on_success_;
      if (func)
      {
        func();
      }
    }
  }

  void ConcludeFail()
  {
    if (conclusion_.exchange(FAIL) == NONE)
    {
      auto func = on_fail_;
      if (func)
      {
        func();
      }
    }
  }

  void Fulfill(byte_array_type const &value)
  {
    LOG_STACK_TRACE_POINT;

    value_     = value;
    fulfilled_ = true;
    ConcludeSuccess();
  }

  PromiseImplementation &Then(callback_type func)
  {
    on_success_ = func;
    if (fulfilled_)
    {
      if (!failed_)
      {
        ConcludeSuccess();  // if this is called >1, it will protect itself.
      }
    }
    return *this;
  }
  PromiseImplementation &Else(callback_type func)
  {
    on_fail_ = func;

#if 1
#else
    if (fulfilled_)
    {
      if (failed_ || connection_closed_)
      {
        ConcludeFail();  // if this is called >1, it will protect itself.
      }
    }
#endif
    return *this;
  }

  void Fail(serializers::SerializableException const &excp)
  {
    LOG_STACK_TRACE_POINT;

    exception_ = excp;
#if 0
    failed_    = true;  // Note that order matters here due to threading!
    fulfilled_ = true;
#endif
    ConcludeFail();
  }

  void ConnectionFailed()
  {
    LOG_STACK_TRACE_POINT;

    FETCH_LOG_WARN(LOGGING_NAME,"ConnectionFailed being signalled.");

#if 0
    connection_closed_ = true;
    fulfilled_         = true;  // Note that order matters here due to threading!
#endif
    ConcludeFail();
  }

  serializers::SerializableException exception() const { return exception_; }
  bool                               is_fulfilled() const { return fulfilled_; }
  bool                               has_failed() const { return failed_; }
  bool                               is_connection_closed() const { return connection_closed_; }

  byte_array_type value() const { return value_; }
  uint64_t        id() const { return id_; }

private:
  serializers::SerializableException exception_;
#if 0
  std::atomic<bool>                  connection_closed_{false};
  std::atomic<bool>                  fulfilled_{false};
  std::atomic<bool>                  failed_{false};
#endif
  std::atomic<uint64_t>              id_;
  byte_array_type                    value_;
  std::atomic<Conclusion>            conclusion_{NONE};
  callback_type                      on_success_;
  callback_type                      on_fail_;

  static uint64_t next_promise_id()
  {
    std::lock_guard<fetch::mutex::Mutex> lock(counter_mutex_);
    uint64_t                             promise = promise_counter_;
    ++promise_counter_;
    return promise;
  }

  static promise_counter_type promise_counter_;
  static fetch::mutex::Mutex  counter_mutex_;
};

}  // namespace details

class Promise
{
public:
  using promise_type         = details::PromiseImplementation;
  using promise_counter_type = typename promise_type::promise_counter_type;
  using shared_promise_type  = std::shared_ptr<promise_type>;
  using callback_type = details::PromiseImplementation::callback_type;

  static constexpr char const *LOGGING_NAME = "Promise";

  Promise()
  {
    LOG_STACK_TRACE_POINT;

    reference_ = std::make_shared<promise_type>();
    created_   = std::chrono::system_clock::now();
  }

  Promise(Promise const &) = default;
  Promise(Promise &&) = default;

  Promise &operator=(Promise const &) = default;
  Promise &operator=(Promise &&) = default;

  bool Wait(bool const &throw_exception)
  {
    return Wait(std::numeric_limits<double>::infinity(), throw_exception);
  }

  typedef enum
  {
    OK      = 0,
    FAILED  = 1,
    CLOSED  = 2,
    WAITING = 4,
  } Status;

  Status GetStatus()
  {
    return Status((has_failed() ? 1 : 0) + (is_connection_closed() ? 2 : 0) +
                  (!is_fulfilled() ? 4 : 0));
  }

  static std::string DescribeStatus(Status s)
  {
    const char *states[8] = {"OK.",      "Failed.", "Closed.", "Failed.",
                             "Timeout.", "Failed.", "Closed.", "Failed."};
    return states[int(s) & 0x07];
  }

  Status WaitLoop(int milliseconds, int cycles)
  {
    auto s = GetStatus();
    while (cycles > 0 && s == WAITING)
    {
      cycles -= 1;
      s = GetStatus();
      if (s != WAITING) return s;

      FETCH_LOG_PROMISE();
      Wait(milliseconds, false);
      s = GetStatus();
      if (s != WAITING) return s;
    }
    return WAITING;
  }

  bool Wait(int const &time) { return Wait(double(time)); }

  //// TODO(EJF): why in the world is this timeout a double?
  bool Wait(double const &timeout = std::numeric_limits<double>::infinity(),
            bool throw_exception  = true)
  {
    LOG_STACK_TRACE_POINT;

    while (!is_fulfilled())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
      double                                ms =
          double(std::chrono::duration_cast<std::chrono::milliseconds>(end - created_).count());
      if ((ms > timeout) && (!is_fulfilled()))
      {
        FETCH_LOG_WARN(LOGGING_NAME,"Connection timed out! ", ms, " vs. ", timeout);
        return false;
      }
    }

    if (is_connection_closed())
    {
      return false;
    }

    if (has_failed())
    {
      FETCH_LOG_WARN(LOGGING_NAME,"Connection failed!");

      if (throw_exception)
        throw reference_->exception();
      else
        return false;
    }
    return true;
  }

  template <typename T>
  T As()
  {
    LOG_STACK_TRACE_POINT;

    FETCH_LOG_PROMISE();
    if (!Wait())
    {
      TODO_FAIL("Timeout or connection lost");
    }

    serializer_type ser(reference_->value());
    T               ret;
    ser >> ret;
    return ret;
  }

  template <typename T>
  void As(T &ret)
  {
    LOG_STACK_TRACE_POINT;

    FETCH_LOG_PROMISE();
    if (!Wait())
    {
      TODO_FAIL("Timeout or connection lost");
    }

    serializer_type ser(reference_->value());
    ser >> ret;
  }

  template <typename T>
  void As(T &ret) const
  {
    LOG_STACK_TRACE_POINT;
    if (!is_fulfilled())
    {
      TODO_FAIL("Don't call non-waity As until promise filled");
    }

    serializer_type ser(reference_->value());
    ser >> ret;
  }

  template <typename T>
  operator T()
  {
    LOG_STACK_TRACE_POINT;
    return As<T>();
  }

  bool     is_fulfilled() const { return reference_->is_fulfilled(); }
  bool     has_failed() const { return reference_->has_failed(); }
  bool     is_connection_closed() const { return reference_->is_connection_closed(); }
  Promise &Then(callback_type func)
  {
    reference_->Then(func);
    return *this;
  }
  Promise &Else(callback_type func)
  {
    reference_->Else(func);
    return *this;
  }

  shared_promise_type  reference() { return reference_; }
  promise_counter_type id() const { return reference_->id(); }

private:
  shared_promise_type                   reference_;
  std::chrono::system_clock::time_point created_;
};

#endif

}  // namespace service
}  // namespace fetch
