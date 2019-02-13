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

  using Callback            = std::function<State(State /*current*/)>;
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
  callbacks_[state] = [instance, func, this](S state) {
    return (instance->*func)(state, previous_state_);
  };
}

template <typename S>
template <typename C>
void StateMachine<S>::RegisterHandler(S state, C *instance, S (C::*func)(S /*current*/))
{
  FETCH_LOCK(callbacks_mutex_);
  callbacks_[state] = [instance, func](S state) { return (instance->*func)(state); };
}

template <typename S>
template <typename C>
void StateMachine<S>::RegisterHandler(S state, C *instance, S (C::*func)())
{
  FETCH_LOCK(callbacks_mutex_);
  callbacks_[state] = [instance, func](S) { return (instance->*func)(); };
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
    S const next_state = it->second(current_state_);

    // check to see if the state has been updated
    if (next_state != current_state_)
    {
      // state has been updated, trigger the update
      previous_state_ = current_state_.load();
      current_state_  = next_state;

      // trigger the state change callback if configured
      if (state_change_callback_)
      {
        state_change_callback_(current_state_.load(), previous_state_.load());
      }
    }
#if 0
    else
    {
      // state has not been updated signal that we should plan the execution for a later time point.
      next_execution_ = Clock::now() + std::chrono::milliseconds{10};
    }
#endif
  }
}

}  // namespace core
}  // namespace fetch
