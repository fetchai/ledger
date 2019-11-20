#pragma once

#include "core/serializers/group_definitions.hpp"
#include "kademlia/primitives.hpp"

#include <chrono>
#include <deque>
#include <vector>

namespace fetch {
namespace muddle {

struct PeerInfo
{
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;
  using Address   = Packet::Address;

  Timepoint first_seen{Clock::now()};
  Timepoint last_activity{};

  /// Serializable fields
  /// @{
  Address          address{};
  KademliaAddress  kademlia_address;
  KademliaDistance distance{};
  std::string      uri{};
  /// @}

  bool is_kademlia_node{false};

  // Promise ping_promise;
  // TimeSpan latency{}
  uint64_t message_count{0};
  uint64_t failures{0};
  bool     unreachable{false};

  bool operator<(PeerInfo const &other) const
  {
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