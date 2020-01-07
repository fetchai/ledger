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

class PromiseError;

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
  static const uint64_t             INVALID = uint64_t(-1);

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
  bool Wait(bool throw_exception = true, uint64_t extend_wait_by = 0) const;

  template <typename T>
  bool GetResult(T &ret, uint64_t extend_wait_by = 0) const;
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
  uint64_t const      protocol_ = INVALID;
  uint64_t const      function_ = INVALID;
  mutable AtomicState state_{State::WAITING};
  ConstByteArray      value_;
  ExceptionPtr        exception_;
  std::string         name_;

  mutable Mutex    callback_lock_;
  mutable Callback callback_success_;
  mutable Callback callback_failure_;
  mutable Callback callback_completion_;

  mutable std::mutex notify_lock_;
  mutable Condition  notify_;
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

class PromiseError final : public std::exception
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using Counter        = details::PromiseImplementation::Counter;
  using Timepoint      = details::PromiseImplementation::Timepoint;
  using State          = details::PromiseImplementation::State;

  explicit PromiseError(details::PromiseImplementation const &promise);
  PromiseError(PromiseError const &other) noexcept;
  ~PromiseError() noexcept override = default;

  char const *what() const noexcept override;

private:
  static std::string BuildErrorMessage(PromiseError const &error);

  Counter const     id_;
  Timepoint const   created_;
  Timepoint const   deadline_;
  uint64_t const    protocol_;
  uint64_t const    function_;
  State const       state_;
  std::string const name_;
  std::string const message_;
};

template <typename T>
bool details::PromiseImplementation::GetResult(T &ret, uint64_t extend_wait_by) const
{
  bool success{false};

  try
  {
    if (Wait(true, extend_wait_by))
    {
      SerializerType ser(value_);
      ser >> ret;

      success = true;
    }
  }
  catch (std::exception const &ex)
  {
    PromiseError const error{*this};
    FETCH_LOG_WARN("Promise", error.what(), " : ", ex.what());
  }

  return success;
}

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
