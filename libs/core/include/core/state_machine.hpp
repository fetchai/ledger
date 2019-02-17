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

#include "core/logger.hpp"
#include "core/mutex.hpp"
#include "core/runnable.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace fetch {
namespace core {

template <typename State>
class StateMachine : public Runnable
{
public:
  static_assert(std::is_enum<State>::value, "");

  using Callback            = std::function<State(State /*current*/, State /*previous*/)>;
  using StateChangeCallback = std::function<void(State /*current*/, State /*previous*/)>;

  // Construction / Destruction
  explicit StateMachine(std::string const &name, State initial);
  StateMachine(StateMachine const &)     = delete;
  StateMachine(StateMachine &&) noexcept = delete;
  ~StateMachine() override               = default;

  /// @name State Control
  /// @{
  template <typename C>
  void RegisterHandler(State state, C *instance,
                       State (C::*func)(State /*current*/, State /*previous*/));
  template <typename C>
  void RegisterHandler(State state, C *instance, State (C::*func)(State /*current*/));
  template <typename C>
  void RegisterHandler(State state, C *instance, State (C::*func)());

  void Reset();

  void OnStateChange(StateChangeCallback cb);
  /// @}

  /// @name Runnable Interface
  /// @{
  bool IsReadyToExecute() const override;
  void Execute() override;
  /// @}

  State state() const
  {
    return current_state_;
  }

  // Operators
  StateMachine &operator=(StateMachine const &) = delete;
  StateMachine &operator=(StateMachine &&) = delete;

private:
  using Clock       = std::chrono::high_resolution_clock;
  using Timepoint   = Clock::time_point;
  using CallbackMap = std::unordered_map<State, Callback>;
  using Mutex       = mutex::Mutex;

  std::string const   logging_name_;
  Mutex               callbacks_mutex_{__LINE__, __FILE__};
  CallbackMap         callbacks_{};
  std::atomic<State>  current_state_;
  std::atomic<State>  previous_state_{current_state_.load()};
  Timepoint           next_execution_{};
  StateChangeCallback state_change_callback_{};
};

template <typename S>
StateMachine<S>::StateMachine(std::string const &name, S initial)
  : logging_name_{"SM:" + name}
  , current_state_{initial}
{}

template <typename S>
template <typename C>
void StateMachine<S>::RegisterHandler(S state, C *instance,
                                      S (C::*func)(S /*current*/, S /*previous*/))
{
  FETCH_LOCK(callbacks_mutex_);
  callbacks_[state] = [instance, func](S state, S prev) { return (instance->*func)(state, prev); };
}

template <typename S>
template <typename C>
void StateMachine<S>::RegisterHandler(S state, C *instance, S (C::*func)(S /*current*/))
{
  FETCH_LOCK(callbacks_mutex_);
  callbacks_[state] = [instance, func](S state, S prev) {
    FETCH_UNUSED(prev);
    return (instance->*func)(state);
  };
}

template <typename S>
template <typename C>
void StateMachine<S>::RegisterHandler(S state, C *instance, S (C::*func)())
{
  FETCH_LOCK(callbacks_mutex_);
  callbacks_[state] = [instance, func](S state, S prev) {
    FETCH_UNUSED(state);
    FETCH_UNUSED(prev);
    return (instance->*func)();
  };
}

template <typename S>
void StateMachine<S>::Reset()
{
  FETCH_LOCK(callbacks_mutex_);
  callbacks_.clear();
  state_change_callback_ = StateChangeCallback{};
}
template <typename S>
void StateMachine<S>::OnStateChange(StateChangeCallback cb)
{
  FETCH_LOCK(callbacks_mutex_);
  state_change_callback_ = std::move(cb);
}

/**
 * Determine if the runnable is ready to the run again
 * @tparam S
 * @return
 */
template <typename S>
bool StateMachine<S>::IsReadyToExecute() const
{
  bool ready{true};

  if (next_execution_.time_since_epoch().count())
  {
    ready = (next_execution_ >= Clock::now());
  }

  return ready;
}

template <typename S>
void StateMachine<S>::Execute()
{
  FETCH_LOCK(callbacks_mutex_);

  // loop up the current state event callback map
  auto it = callbacks_.find(current_state_);
  if (it != callbacks_.end())
  {
    // execute the state handler
    S const next_state = it->second(current_state_, previous_state_);

    // perform the state updates
    previous_state_ = current_state_.load();
    current_state_  = next_state;

    // detect a state change
    if (current_state_ != previous_state_)
    {
      // trigger the state change callback if configured
      if (state_change_callback_)
      {
        state_change_callback_(current_state_, previous_state_);
      }
    }
  }
}

}  // namespace core
}  // namespace fetch
