#pragma once

#include "muddle/kademlia/primitives.hpp"

#include <chrono>
#include <deque>
#include <vector>

namespace fetch {
namespace muddle {

struct PeerInfo
{
  using Clock      = std::chrono::steady_clock;
  using Timepoint  = Clock::time_point;
  using RawAddress = Packet::RawAddress;

  Timepoint first_seen{Clock::now()};
  Timepoint last_activity{};

  RawAddress       address{};
  KademliaAddress  kademlia_address;
  KademliaDistance distance{};
  bool             is_kademlia_node{false};

  Uri uri{};

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
}  // namespace fetch