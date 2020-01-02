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

#include "core/serializers/main_serializer.hpp"

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
  using AccurateSystemClock = std::chrono::system_clock;
  using Timestamp           = AccurateSystemClock::time_point;
  using Duration            = AccurateSystemClock::duration;

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

namespace serializers {

template <typename D>
struct ForwardSerializer<moment::ClockInterface::Duration, D>
{
public:
  using Type       = moment::ClockInterface::Duration;
  using DriverType = D;

  template <typename Serializer>
  static void Serialize(Serializer &serializer, Type const &item)
  {
    serializer << static_cast<uint64_t>(item.count());
  }

  template <typename Serializer>
  static void Deserialize(Serializer &deserializer, Type &item)
  {
    uint64_t time;
    deserializer >> time;
    item = Type(time);
  }
};

template <typename D>
struct ForwardSerializer<moment::ClockInterface::Timestamp, D>
{
public:
  using Type       = moment::ClockInterface::Timestamp;
  using DriverType = D;

  template <typename Serializer>
  static void Serialize(Serializer &serializer, Type const &item)
  {
    auto tse = item.time_since_epoch();
    serializer << tse;
  }

  template <typename Serializer>
  static void Deserialize(Serializer &deserializer, Type &item)
  {
    moment::ClockInterface::Duration tse;
    deserializer >> tse;
    item = Type(tse);
  }
};
}  // namespace serializers

}  // namespace fetch
