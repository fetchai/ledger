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

#include "core/serializers/group_definitions.hpp"
#include "kademlia/primitives.hpp"
#include "moment/clock_interfaces.hpp"
#include "network/uri.hpp"

#include <chrono>
#include <deque>
#include <mutex>
#include <vector>

namespace fetch {
namespace muddle {

struct PeerInfo
{
  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;
  using Duration       = ClockInterface::Duration;

  using Address = Packet::Address;
  using Uri     = network::Uri;

  /// Meta information
  /// @{
  Timepoint connection_start{Clock::now()};
  Timepoint last_activity{};
  Timepoint earliest_next_attempt{Clock::now()};
  uint64_t  connection_attempts{0};
  uint64_t  failed_attempts{0};
  uint64_t  connections{0};
  /// @}

  /// Serializable fields
  /// @{
  bool             verified{false};
  Address          address{};
  KademliaAddress  kademlia_address;
  KademliaDistance distance{};
  Uri              uri{};
  /// @}

  Address  last_reporter{};
  uint64_t message_count{0};

  bool CanConnect() const
  {
    return earliest_next_attempt < Clock::now();
  }

  bool operator<(PeerInfo const &other) const
  {

    if (verified != other.verified)
    {
      return verified;
    }

    // Assumes a little endian type structure
    uint64_t i = distance.size();
    while (i != 0)
    {
      --i;
      if (distance[i] != other.distance[i])
      {
        break;
      }
    }

    return distance[i] < other.distance[i];
  }
};

}  // namespace muddle

namespace serializers {

template <typename D>
struct MapSerializer<muddle::PeerInfo, D>
{
public:
  using Type       = muddle::PeerInfo;
  using DriverType = D;

  static uint8_t const ADDRESS           = 0;
  static uint8_t const KADEMLIA_ADDRESS  = 1;
  static uint8_t const KADEMLIA_DISTANCE = 2;
  static uint8_t const URI               = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &info)
  {
    auto map = map_constructor(4);

    map.Append(ADDRESS, info.address);
    map.Append(KADEMLIA_ADDRESS, info.kademlia_address);
    map.Append(KADEMLIA_DISTANCE, info.distance);
    map.Append(URI, info.uri);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &info)
  {
    map.ExpectKeyGetValue(ADDRESS, info.address);
    map.ExpectKeyGetValue(KADEMLIA_ADDRESS, info.kademlia_address);
    map.ExpectKeyGetValue(KADEMLIA_DISTANCE, info.distance);
    map.ExpectKeyGetValue(URI, info.uri);
  }
};

}  // namespace serializers

}  // namespace fetch
