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
#include "network/service/promise.hpp"

namespace fetch {
namespace muddle {

struct RouterConfiguration
{
  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;
  using Duration       = ClockInterface::Duration;

  uint64_t max_delivery_attempts{3};
  Duration temporary_connection_length{
      std::chrono::seconds(4)};  ///< Time should be slightly longer than the retry period
  uint32_t retry_delay_ms{2000};
};

}  // namespace muddle
}  // namespace fetch
