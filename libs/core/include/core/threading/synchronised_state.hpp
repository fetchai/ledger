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

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace fetch {

template <typename S>
class SynchronisedState
{
public:
  using State = S;

  // Construction / Destruction
  SynchronisedState() = default;
  explicit SynchronisedState(State const &initial);
  explicit SynchronisedState(State &&initial);
  SynchronisedState(SynchronisedState const &) = delete;
  SynchronisedState(SynchronisedState &&)      = delete;
  ~SynchronisedState()                         = default;

  /// @name Normal Access
  /// @{
  State Get() const;
  void  Set(State const &state);
  void  Set(State &&state);
  /// @}

  /// @name State Specific Updates
  /// @{
  template <typename R, typename P>
  bool WaitFor(State const &state, std::chrono::duration<R, P> const &max_wait_time) const;
  void WaitFor(State const &state) const;

  template <typename Handler>
  void WaitFor(Handler &&handler) const;

  template <typename Handler, typename R, typename P>
  bool WaitFor(std::chrono::duration<R, P> const &max_wait_time, Handler &&handler) const;

  template <typename Handler>
  void Apply(Handler &&handler);
  template <typename Handler>
  void Apply(Handler &&handler) const;
  /// @}

  // Operators
  SynchronisedState &operator=(SynchronisedState const &) = delete;
  SynchronisedState &operator=(SynchronisedState &&) = delete;

private:
  using Mutex      = std::mutex;
  using Condition  = std::condition_variable;
  using UniqueLock = std::unique_lock<Mutex>;

  mutable Mutex     lock_{};       ///< The lock protecting the state
  mutable Condition condition_{};  ///< The condition variable to notify
  State             state_;        ///< The current state of the variable
};

template <typename S>
SynchronisedState<S>::SynchronisedState(State const &initial)
  : state_{initial}
{}

template <typename S>
SynchronisedState<S>::SynchronisedState(State &&initial)
  : state_{std::move(initial)}
{}

template <typename S>
S SynchronisedState<S>::Get() const
{
  FETCH_LOCK(lock_);
  return state_;
}

template <typename S>
void SynchronisedState<S>::Set(State const &state)
{
  {
    FETCH_LOCK(lock_);
    state_ = state;
  }
  condition_.notify_all();
}

template <typename S>
void SynchronisedState<S>::Set(State &&state)
{
  {
    FETCH_LOCK(lock_);
    state_ = std::move(state);
  }
  condition_.notify_all();
}

template <typename S>
template <typename R, typename P>
bool SynchronisedState<S>::WaitFor(State const &                      state,
                                   std::chrono::duration<R, P> const &max_wait_time) const
{
  UniqueLock lock{lock_};
  return condition_.wait_for(lock, max_wait_time, [this, &state]() { return state_ == state; });
}

template <typename S>
void SynchronisedState<S>::WaitFor(State const &state) const
{
  UniqueLock lock{lock_};
  condition_.wait(lock, [this, &state]() { return state_ == state; });
}

template <typename S>
template <typename Handler>
void SynchronisedState<S>::WaitFor(Handler &&handler) const
{
  UniqueLock lock{lock_};
  condition_.wait(lock, [this, h = std::forward<Handler>(handler)]() { return h(state_); });
}

template <typename S>
template <typename Handler, typename R, typename P>
bool SynchronisedState<S>::WaitFor(std::chrono::duration<R, P> const &max_wait_time,
                                   Handler &&                         handler) const
{
  UniqueLock lock{lock_};
  return condition_.wait_for(lock, max_wait_time,
                             [this, h = std::forward<Handler>(handler)]() { return h(state_); });
}

template <typename S>
template <typename Handler>
void SynchronisedState<S>::Apply(Handler &&handler)
{
  {
    FETCH_LOCK(lock_);
    handler(state_);
  }
  condition_.notify_all();
}

template <typename S>
template <typename Handler>
void SynchronisedState<S>::Apply(Handler &&handler) const
{
  {
    FETCH_LOCK(lock_);
    handler(state_);
  }
  condition_.notify_all();
}

}  // namespace fetch
