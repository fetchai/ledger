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
#include <memory>

namespace fetch {
namespace moment {

/**
 * Basic Clock Interface
 */
class ClockInterface
{
public:
  using ChronoClock = std::chrono::steady_clock;
  using Timestamp   = ChronoClock::time_point;
  using Duration    = ChronoClock::duration;

  // Construction / Destruction
  ClockInterface()          = default;
  virtual ~ClockInterface() = default;

  /// @name Clock Interface
  /// @{

  /**
   * Get the current time of the clock
   *
   * @return The current timestamp
   */
  virtual Timestamp Now() const = 0;

  /// @}
};

/**
 * Adjustable Clock Interface
 */
class AdjustableClockInterface : public ClockInterface
{
public:
  // Construction / Destruction
  AdjustableClockInterface()           = default;
  ~AdjustableClockInterface() override = default;

  /// @name Adjustable ClockInterface
  /// @{

  /**
   * Add an additional offset to the clock
   *
   * @param duration The offset to apply to the clock
   */
  virtual void AddOffset(Duration const &duration) = 0;

  /// @}

  // Helpers
  template <typename R, typename P>
  void Advance(std::chrono::duration<R, P> const &duration);
};

/**
 * Advance the clock by a specified duration
 *
 * @tparam R The representation of the duration
 * @tparam P The period of the duration
 * @param duration The offset to be applied to the clock
 */
template <typename R, typename P>
void AdjustableClockInterface::Advance(std::chrono::duration<R, P> const &duration)
{
  AddOffset(std::chrono::duration_cast<Duration>(duration));
}

using ClockPtr           = std::shared_ptr<ClockInterface>;
using AdjustableClockPtr = std::shared_ptr<AdjustableClockInterface>;

}  // namespace moment
}  // namespace fetch
