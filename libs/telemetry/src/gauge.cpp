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

#include "telemetry/gauge.hpp"
#include "telemetry/measurement.hpp"

#include <iomanip>
#include <iostream>
#include <mutex>
#include <type_traits>

namespace fetch {
namespace telemetry {

/**
 * Internal: int8_t specific value formatter
 *
 * @param gauge The reference to the current gauge
 * @param stream The stream to be populated
 */
void GaugeToStream(Gauge<int8_t> const &gauge, OutputStream &stream)
{
  stream << static_cast<int32_t>(gauge.get()) << '\n';
}

/**
 * Internal: uint8_t specific value formatter
 *
 * @param gauge The reference to the current gauge
 * @param stream The stream to be populated
 */
void GaugeToStream(Gauge<uint8_t> const &gauge, OutputStream &stream)
{
  stream << static_cast<uint32_t>(gauge.get()) << '\n';
}

}  // namespace telemetry
}  // namespace fetch
