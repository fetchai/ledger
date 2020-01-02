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

#include "core/macros.hpp"
#include "core/runnable.hpp"
#include "core/state_machine_interface.hpp"
#include "core/synchronisation/protected.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

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
  static_assert(std::is_enum<State>::value, "");

  using Callback                     = std::function<State(State /*current*/, State /*previous*/)>;
  using StateChangeCallback          = std::function<void(State /*current*/, State /*previous*/)>;
  using StateMapper                  = std::function<char const *(State)>;
  using ProtectedStateChangeCallback = Protected<StateChangeCallback>;

public:
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

  void OnStateChange(StateChangeCallback cb);
  /// @}

  /// @name State Machine Interface
  /// @{
  char const *GetName() const override;
  char const *GetStateName() const override;
  /// @}

  /// @name Runnable Interface
  /// @{
  bool        IsReadyToExecute() const override;
  void        Execute() override;
  char const *GetId() const override;
  /// @}

  State state() const
  {
    return current_state_;
  }

  State previous_state() const
  {
    return previous_state_;
  }

  template <typename R, typename P>
  void Delay(std::chrono::duration<R, P> const &delay);

  // Operators
  StateMachine &operator=(StateMachine const &) = delete;
  StateMachine &operator=(StateMachine &&) = delete;

private:
  using Clock                = std::chrono::steady_clock;
  using Timepoint            = Clock::time_point;
  using Duration             = Clock::duration;
  using CallbackMap          = std::unordered_map<State, Callback>;
  using ProtectedCallbackMap = Protected<CallbackMap>;

  static std::string ToLowerCase(std::string data)
  {
    std::transform(data.begin(), data.end(), data.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return data;
  }

  void Reset();

  std::string const             name_;
  StateMapper                   mapper_;
  ProtectedCallbackMap          callbacks_{};
  std::atomic<State>            current_state_;
  std::atomic<State>            previous_state_{current_state_.load()};
  Timepoint                     next_execution_{};
  ProtectedStateChangeCallback  state_change_callback_{};
  telemetry::GaugePtr<uint64_t> state_gauge_;
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
  , state_gauge_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        ToLowerCase(name_) + "_state_gauge", "Generic state machine state as integer")}
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
  callbacks_.ApplyVoid([func, instance, state](auto &callbacks) {
    callbacks[state] = [func, instance](S state, S prev) { return (instance->*func)(state, prev); };
  });
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
  callbacks_.ApplyVoid([func, instance, state](auto &callbacks) {
    callbacks[state] = [func, instance](S state, S prev) {
      FETCH_UNUSED(prev);
      return (instance->*func)(state);
    };
  });
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
  callbacks_.ApplyVoid([func, instance, state](auto &callbacks) {
    callbacks[state] = [func, instance](S state, S prev) {
      FETCH_UNUSED(state);
      FETCH_UNUSED(prev);
      return (instance->*func)();
    };
  });
}

/**
 * Clear all the callbacks associated with this state machine
 *
 * @tparam S The state enum type
 */
template <typename S>
void StateMachine<S>::Reset()
{
  callbacks_.ApplyVoid([](auto &callbacks) { callbacks.clear(); });

  state_change_callback_.ApplyVoid(
      [](auto &state_change_callback) { state_change_callback = StateChangeCallback{}; });
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
  state_change_callback_.ApplyVoid(
      [&cb](auto &state_change_callback) { state_change_callback = std::move(cb); });
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
 * Get the id of the runnable
 *
 * @tparam S The state enum type
 * @return The string name of the state machine
 */
template <typename S>
char const *StateMachine<S>::GetId() const
{
  return name_.c_str();
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
  callbacks_.ApplyVoid([this](auto &callbacks) {
    // iterate over the current state event callback map
    auto it = callbacks.find(current_state_);
    if (it != callbacks.end())
    {
      // execute the state handler
      S const next_state = it->second(current_state_, previous_state_);

      // perform the state updates
      previous_state_ = current_state_.load();
      state_gauge_->set(static_cast<uint64_t>(next_state));
      current_state_ = next_state;

      // detect a state change
      if (current_state_ != previous_state_)
      {
        // trigger the state change callback if configured
        state_change_callback_.ApplyVoid([this](auto &state_change_callback) {
          if (state_change_callback)
          {
            state_change_callback(current_state_, previous_state_);
          }
        });
      }
    }
  });
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
