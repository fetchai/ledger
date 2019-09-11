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
#include "logging/logging.hpp"
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
  PromiseImplementation() = default;
  PromiseImplementation(uint64_t pro, uint64_t func);

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

  ConstByteArray const &       value() const;
  Counter                      id() const;
  uint64_t                     protocol() const;
  uint64_t                     function() const;
  State                        state() const;
  SerializableException const &exception() const;

  /// @name State Access
  /// @{
  bool IsWaiting() const;
  bool IsSuccessful() const;
  bool IsFailed() const;
  /// @}
private:
  /// @name Callback Handlers
  /// @{
  void SetSuccessCallback(Callback const &cb);
  void SetFailureCallback(Callback const &cb);
  void SetCompletionCallback(Callback const &cb);
  /// @}

  uint64_t protocol_ = uint64_t(-1);
  uint64_t function_ = uint64_t(-1);

public:
  PromiseBuilder WithHandlers();

  /// @name Promise Results
  /// @{
  void Fulfill(ConstByteArray const &value);
  void Fail(SerializableException const &exception);
  void Fail();
  /// @}

  std::string &      name();
  const std::string &name() const;

  State GetState() const;

  /// @name Waits
  /// @{
  bool Wait(uint32_t timeout_ms = FOREVER, bool throw_exception = true) const;
  bool Wait(bool throw_exception) const;
  /// @}

  /// @name Result Access
  /// @{
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

    serializer_type ser(value_);
    ser >> ret;

    return true;
  }
  /// @}

private:
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

  mutable Mutex callback_lock_;
  Callback      callback_success_;
  Callback      callback_failure_;
  Callback      callback_completion_;

#define FETCH_PROMISE_CV
#ifdef FETCH_PROMISE_CV
  mutable Mutex     notify_lock_;
  mutable Condition notify_;
#endif
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
