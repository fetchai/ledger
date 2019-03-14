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
  // Construction / Destruction
  template <typename R, typename P>
  explicit PeriodicAction(std::chrono::duration<R, P> const &period);
  PeriodicAction(PeriodicAction const &) = delete;
  PeriodicAction(PeriodicAction &&)      = default;
  ~PeriodicAction()                      = default;

  bool Poll();
  void Reset();

  // Operators
  PeriodicAction &operator=(PeriodicAction const &) = delete;
  PeriodicAction &operator=(PeriodicAction &&) = default;

private:
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using Duration  = Clock::duration;

  Duration      period_;
  Timepoint     start_time_;
  Duration::rep last_index_{0};
};

/**
 * Construct an action based on a period and a callback
 *
 * @tparam R The type of the representation
 * @tparam P The type fo the period
 * @param period The minimum period for which the action should be called
 */
template <typename R, typename P>
PeriodicAction::PeriodicAction(std::chrono::duration<R, P> const &period)
  : period_{std::chrono::duration_cast<Duration>(period)}
  , start_time_{Clock::now() + period_}
{}

/**
 * Called periodically to trigger the action is needed
 */
inline bool PeriodicAction::Poll()
{
  bool triggered{false};

  Duration::rep const current_index = ((Clock::now() - start_time_) / period_);

  if (current_index > last_index_)
  {
    // update the index
    last_index_ = current_index;

    // signal that the action has been triggered
    triggered = true;
  }

  return triggered;
}

inline void PeriodicAction::Reset()
{
  start_time_ = Clock::now();
}

}  // namespace fetch