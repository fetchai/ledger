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
#include "moment/deadline_timer.hpp"

#include <chrono>
#include <cstdint>

namespace fetch {
namespace moment {

DeadlineTimer::DeadlineTimer(char const *clock)
  : clock_{GetClock(clock, ClockType::SYSTEM)}
{}

void DeadlineTimer::Restart(uint64_t period_ms)
{
  Restart(std::chrono::milliseconds{period_ms});
}

bool DeadlineTimer::HasExpired() const
{
  return deadline_ <= clock_->Now();
}

}  // namespace moment
}  // namespace fetch
