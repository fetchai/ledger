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

#include "moment/clocks.hpp"

#include <cstdint>

namespace fetch {
namespace moment {

class DeadlineTimer
{
public:
  // Construction / Destruction
  explicit DeadlineTimer(char const *clock);
  DeadlineTimer(DeadlineTimer const &) = default;
  DeadlineTimer(DeadlineTimer &&)      = default;
  ~DeadlineTimer()                     = default;

  template <typename R, typename P>
  void Restart(std::chrono::duration<R, P> const &period);
  void Restart(uint64_t period_ms);

  bool HasExpired() const;

  template <typename Duration>
  uint64_t GetTimeRemaining() const
  {
    auto const now = clock_->Now();
    if (deadline_ > now)
    {
      return static_cast<uint64_t>(std::chrono::duration_cast<Duration>(deadline_ - now).count());
    }

    return 0;
  }

  // Operators
  DeadlineTimer &operator=(DeadlineTimer const &) = default;
  DeadlineTimer &operator=(DeadlineTimer &&) = default;

private:
  using Timestamp = ClockInterface::Timestamp;

  ClockPtr  clock_;
  Timestamp deadline_{};
};

template <typename R, typename P>
void DeadlineTimer::Restart(std::chrono::duration<R, P> const &period)
{
  deadline_ = clock_->Now() + period;
}

}  // namespace moment
}  // namespace fetch
