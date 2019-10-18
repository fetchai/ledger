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
#include "core/mutex.hpp"
#include "core/serializers/exception.hpp"
#include "logging/logging.hpp"
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

namespace serializers {
class SerializableException;
}

namespace service {

namespace details {

class PromiseBuilder;

class PromiseImplementation
{
  friend PromiseBuilder;

public:
  using Counter        = uint64_t;
  using ConstByteArray = byte_array::ConstByteArray;
  using ExceptionPtr   = std::unique_ptr<serializers::SerializableException>;
  using Callback       = std::function<void()>;
  using Clock          = std::chrono::steady_clock;
  using Timepoint      = Clock::time_point;
  using Duration       = Clock::duration;

  enum class State
  {
    WAITING,
    SUCCESS,
    FAILED,
    TIMEDOUT
  };

  static constexpr char const *     LOGGING_NAME = "Promise";
  static std::chrono::seconds const DEFAULT_TIMEOUT;

  // Construction / Destruction
  PromiseImplementation() = default;
  PromiseImplementation(uint64_t protocol, uint64_t function);
  PromiseImplementation(PromiseImplementation const &) = delete;
  PromiseImplementation(PromiseImplementation &&)      = delete;
  ~PromiseImplementation()                             = default;

  /// @name Accessors
  /// @{
  ConstByteArray const &value() const;
  Counter               id() const;
  Timepoint const &     created_at() const;
  Timepoint const &     deadline() const;
  uint64_t              protocol() const;
  uint64_t              function() const;
  State                 state() const;
  std::string const &   name() const;
  /// @}

  /// @name Basic State Helpers
  /// @{
  bool IsWaiting() const;
  bool IsSuccessful() const;
  bool IsFailed() const;
  /// @}

  // Handler building
  PromiseBuilder WithHandlers();

  /// @name Promise Results
  /// @{
  void Fulfill(ConstByteArray const &value);
  void Fail(serializers::SerializableException const &exception);
  void Timeout();
  void Fail();
  /// @}

  /// @name Result Access
  /// @{
  bool Wait(bool throw_exception = true) const;

  template <typename T>
  T As() const
  {
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
    if (!Wait())
    {
      return false;
    }

    SerializerType ser(value_);
    ser >> ret;

    return true;
  }
  /// @}

  // Operators
  PromiseImplementation &operator=(PromiseImplementation const &) = delete;
  PromiseImplementation &operator=(PromiseImplementation &&) = delete;

protected:
  /// @name Callback Handlers
  /// @{
  void SetSuccessCallback(Callback const &cb);
  void SetFailureCallback(Callback const &cb);
  void SetCompletionCallback(Callback const &cb);
  /// @}

private:
  using AtomicState = std::atomic<State>;
  using Condition   = std::condition_variable;

  void UpdateState(State state) const;
  void DispatchCallbacks() const;

  static Counter counter_;
  static Mutex   counter_lock_;
  static Counter GetNextId();

  Counter const       id_{GetNextId()};
  Timepoint const     created_{Clock::now()};
  Timepoint const     deadline_{created_ + DEFAULT_TIMEOUT};
  uint64_t const      protocol_ = uint64_t(-1);
  uint64_t const      function_ = uint64_t(-1);
  mutable AtomicState state_{State::WAITING};
  ConstByteArray      value_;
  ExceptionPtr        exception_;
  std::string         name_;

  mutable Mutex    callback_lock_;
  mutable Callback callback_success_;
  mutable Callback callback_failure_;
  mutable Callback callback_completion_;

  mutable Mutex     notify_lock_;
  mutable Condition notify_;
};

class PromiseBuilder
{
public:
  using Callback = PromiseImplementation::Callback;

  explicit PromiseBuilder(PromiseImplementation &promise);

  ~PromiseBuilder();

  PromiseBuilder &Then(Callback const &cb);
  PromiseBuilder &Catch(Callback const &cb);
  PromiseBuilder &Finally(Callback const &cb);

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

Promise MakePromise();
Promise MakePromise(uint64_t pro, uint64_t func);

char const *         ToString(PromiseState state);
const PromiseStates &GetAllPromiseStates();

}  // namespace service
}  // namespace fetch
