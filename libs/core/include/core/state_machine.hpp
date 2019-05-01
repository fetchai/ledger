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
#include "core/state_machine_interface.hpp"

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace fetch {
namespace core {

/**
 * Finite State Machine
 *
 * This class is used as a simple helper around a series of callbacks to functions that handle the
 * currently assigned state.
 *
 * It is expected that this class is created by the parent class using std::make_shared() or
 * similar. In general, these state machines should be executed by a reactor.
 *
 * @tparam State the enum state type
 */
template <typename State>
class StateMachine : public StateMachineInterface, public Runnable
{
public:
  static_assert(std::is_enum<State>::value, "");

  using Callback            = std::function<State(State /*current*/, State /*previous*/)>;
  using StateChangeCallback = std::function<void(State /*current*/, State /*previous*/)>;
  using StateMapper         = std::function<char const *(State)>;

  // Construction / Destruction
  explicit StateMachine(std::string name, State initial, StateMapper mapper = StateMapper{});
  StateMachine(StateMachine const &)     = delete;
  StateMachine(StateMachine &&) noexcept = delete;
  ~StateMachine() override;

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

  /// @name State Machine Interface
  /// @{
  char const *GetName() const override;
  uint64_t    GetStateCode() const override;
  char const *GetStateName() const override;
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

  template <typename R, typename P>
  void Delay(std::chrono::duration<R, P> const &delay);

  // Operators
  StateMachine &operator=(StateMachine const &) = delete;
  StateMachine &operator=(StateMachine &&) = delete;

private:
  using Clock       = std::chrono::steady_clock;
  using Timepoint   = Clock::time_point;
  using Duration    = Clock::duration;
  using CallbackMap = std::unordered_map<State, Callback>;
  using Mutex       = std::mutex;

  std::string const   name_;
  StateMapper         mapper_;
  mutable Mutex       callbacks_mutex_;
  CallbackMap         callbacks_{};
  std::atomic<State>  current_state_;
  std::atomic<State>  previous_state_{current_state_.load()};
  Timepoint           next_execution_{};
  StateChangeCallback state_change_callback_{};
};

/**
 * Construct instance of the state machine
 *
 * @tparam S The state enum type
 * @param name The name of the state machine
 * @param initial The initial state for the state machine
 * @param mapper An optional mapper to generate a string representation for the state
 */
template <typename S>
StateMachine<S>::StateMachine(std::string name, S initial, StateMapper mapper)
  : name_{std::move(name)}
  , mapper_{std::move(mapper)}
  , current_state_{initial}
{}

/**
 * Destruct and tear down the state handlers for this object
 *
 * @tparam S The state enum type
 */
template <typename S>
StateMachine<S>::~StateMachine()
{
  Reset();
}

/**
 * Register a class member handler for a specified state
 *
 * @tparam S The state enum type
 * @tparam C The class type
 * @param state The state to be trigger on
 * @param instance The instance of the class
 * @param func The member function pointer
 */
template <typename S>
template <typename C>
void StateMachine<S>::RegisterHandler(S state, C *instance,
                                      S (C::*func)(S /*current*/, S /*previous*/))
{
  FETCH_LOCK(callbacks_mutex_);
  callbacks_[state] = [instance, func](S state, S prev) { return (instance->*func)(state, prev); };
}

/**
 * Register a class member handler for a specified state
 *
 * @tparam S The state enum type
 * @tparam C The class type
 * @param state The state to be trigger on
 * @param instance The instance of the class
 * @param func The member function pointer
 */
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

/**
 * Register a class member handler for a specified state
 *
 * @tparam S The state enum type
 * @tparam C The class type
 * @param state The state to be trigger on
 * @param instance The instance of the class
 * @param func The member function pointer
 */
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

/**
 * Clear all the callbacks associated with this state machine
 *
 * @tparam S The state enum type
 */
template <typename S>
void StateMachine<S>::Reset()
{
  FETCH_LOCK(callbacks_mutex_);
  callbacks_.clear();
  state_change_callback_ = StateChangeCallback{};
}

/**
 * Register or update the state change callback for this state machine
 *
 * @tparam S The state enum type
 * @param cb The callback function to be registered
 */
template <typename S>
void StateMachine<S>::OnStateChange(StateChangeCallback cb)
{
  FETCH_LOCK(callbacks_mutex_);
  state_change_callback_ = std::move(cb);
}

/**
 * Get the name of the state machine instance
 *
 * @tparam S The state enum type
 * @return The string name of the state machine
 */
template <typename S>
char const *StateMachine<S>::GetName() const
{
  return name_.c_str();
}

/**
 * Get the current value of the enum state type for the current state
 *
 * @tparam S The state enum type
 * @return The numeric value associated with the state
 */
template <typename S>
uint64_t StateMachine<S>::GetStateCode() const
{
  return static_cast<uint64_t>(state());
}

/**
 * Get the current string representation for the current state
 *
 * @tparam S The state enum type
 * @return The string representation of the current state if successful otherwise "Unknown"
 */
template <typename S>
char const *StateMachine<S>::GetStateName() const
{
  char const *text = "Unknown";

  if (mapper_)
  {
    text = mapper_(state());
  }

  return text;
}

/**
 * Determine if the runnable is ready to the run again
 *
 * @tparam S The state enum type
 * @return true if the state machine should be executed again, otherwise false
 */
template <typename S>
bool StateMachine<S>::IsReadyToExecute() const
{
  bool ready{true};

  if (next_execution_.time_since_epoch().count())
  {
    ready = (Clock::now() >= next_execution_);
  }

  return ready;
}

/**
 * Execute the state machine (called from the reactor)
 *
 * @tparam S The state enum type
 */
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

/**
 * Configure the next execution of the state machine for a future point
 *
 * Note: Function to be called from within the state machine call context
 *
 * @tparam S The type of the state
 * @tparam R The type of the representation
 * @tparam P The type of the period
 * @param delay The delay to be set until the next execution
 */
template <typename S>
template <typename R, typename P>
void StateMachine<S>::Delay(std::chrono::duration<R, P> const &delay)
{
  next_execution_ = Clock::now() + delay;
}

}  // namespace core
}  // namespace fetch
