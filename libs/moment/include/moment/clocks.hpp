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

#include "moment/clock_interfaces.hpp"

namespace fetch {
namespace moment {

enum class ClockType
{
  SYSTEM,
};

enum class TimeAccuracy
{
  SECONDS,
  MILLISECONDS,
};

ClockPtr           GetClock(char const *name, ClockType default_type = ClockType::SYSTEM);
AdjustableClockPtr CreateAdjustableClock(char const *name, ClockType type = ClockType::SYSTEM);

// Convenience function to provide the time as a uint64
uint64_t GetTime(moment::ClockPtr const &clock, TimeAccuracy accuracy = TimeAccuracy::SECONDS);

}  // namespace moment
}  // namespace fetch
