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
#include <functional>

namespace fetch {

/**
 * Lightweight wrapper around a function to be trigger periodically
 *
 * This is not thread-safe and is expected to be used in a context of a single thread or where
 * synchronisation is guarenteed by other means.
 */
class PeriodicAction
{
public:
  using Callback = std::function<void()>;

  // Construction / Destruction
  template <typename R, typename P>
  PeriodicAction(std::chrono::duration<R, P> const &period, Callback callback = Callback{});
  PeriodicAction(PeriodicAction const &) = delete;
  PeriodicAction(PeriodicAction &&)      = default;
  ~PeriodicAction()                      = default;

  void SetCallback(Callback callback);
  bool Poll();

  // Operators
  PeriodicAction &operator=(PeriodicAction const &) = delete;
  PeriodicAction &operator=(PeriodicAction &&) = default;

private:
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using Duration  = Clock::duration;

  Duration  period_;
  Timepoint next_action_time_;
  Callback  callback_;
};

/**
 * Construct an action based on a period and a callback
 *
 * @tparam R The type of the representation
 * @tparam P The type fo the period
 * @param period The minimum period for which the action should be called
 * @param callback A callback function to be called at the given interval
 */
template <typename R, typename P>
PeriodicAction::PeriodicAction(std::chrono::duration<R, P> const &period, Callback callback)
  : period_{std::chrono::duration_cast<Duration>(period)}
  , next_action_time_{Clock::now() + period_}
  , callback_{std::move(callback)}
{}

/**
 * Sets the callback on the action
 *
 * @param callback The callback to be set
 */
inline void PeriodicAction::SetCallback(Callback callback)
{
  callback_ = std::move(callback);
}

/**
 * Called periodically to trigger the action is needed
 */
inline bool PeriodicAction::Poll()
{
  bool triggered{false};

  auto const now = Clock::now();

  if (now >= next_action_time_)
  {
    // execute the callback if it exists
    if (callback_)
    {
      callback_();
    }

    // reset the next action time
    next_action_time_ = now + period_;

    // signal that the action has been triggered
    triggered = true;
  }

  return triggered;
}

}  // namespace fetch