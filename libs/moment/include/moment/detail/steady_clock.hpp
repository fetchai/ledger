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

#include <chrono>

namespace fetch {
namespace moment {
namespace detail {

class SystemClock : public ClockInterface
{
public:
  // Construction / Destruction
  SystemClock()                    = default;
  SystemClock(SystemClock const &) = delete;
  SystemClock(SystemClock &&)      = delete;
  ~SystemClock() override          = default;

  /// @name Clock Interface
  /// @{
  Timestamp Now() const override
  {
    return AccurateSystemClock::now();
  }
  /// @}

  // Operators
  SystemClock &operator=(SystemClock const &) = delete;
  SystemClock &operator=(SystemClock &&) = delete;
};

}  // namespace detail
}  // namespace moment
}  // namespace fetch
