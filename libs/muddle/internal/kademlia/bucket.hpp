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

#include "kademlia/peer_info.hpp"
#include "kademlia/primitives.hpp"
#include "moment/clock_interfaces.hpp"

#include <deque>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace muddle {

struct Bucket
{
  using ClockInterface = moment::ClockInterface;
  using Clock          = ClockInterface::AccurateSystemClock;
  using Timepoint      = ClockInterface::Timestamp;
  using Duration       = ClockInterface::Duration;

  using BucketId = uint64_t;
  using Peer     = std::shared_ptr<PeerInfo>;  // TODO(tfr): Make weak

  BucketId                 bucket_id;
  std::unordered_set<Peer> peers;
  Timepoint                last_updated;  // TODO(tfr): not used atm

  static uint64_t IdByHamming(KademliaDistance const &dist)
  {
    uint64_t ret{0};
    for (auto const &d : dist)
    {
      ret += platform::CountSetBits(d);
    }
    return ret;
  }

  static uint64_t IdByLogarithm(KademliaDistance const &dist)
  {

    static uint64_t zeros[16] = {
        4,  // 0: 0b0000
        3,  // 1: 0b0001
        2,  // 2: 0b0010
        2,  // 3: 0b0011
        1,  // 4: 0b0100
        1,  // 5: 0b0101
        1,  // 6: 0b0110
        1,  // 7: 0b0111
        0,  // 8: 0b1000
        0,  // 9: 0b1001
        0,  // 10: 0b1010
        0,  // 11: 0b1011
        0,  // 12: 0b1100
        0,  // 13: 0b1101
        0,  // 14: 0b1110
        0   // 15: 0b1111
    };

    uint64_t    ret{8 * dist.size() * sizeof(uint8_t)};
    std::size_t i = dist.size();

    // TODO(tfr): this function can be improved by using intrinsics
    // but endianness needs to be considered.
    do
    {
      --i;
      auto top = zeros[(dist[i] >> 4) & 0xF];
      ret -= top;
      if (top == 4)
      {
        ret -= zeros[dist[i] & 0xF];
      }

    } while ((i != 0) && (dist[i] == 0));

    return ret;
  }
};

}  // namespace muddle

namespace serializers {

template <typename D>
struct MapSerializer<muddle::Bucket, D>
{
public:
  using Type       = muddle::Bucket;
  using DriverType = D;

  static uint8_t const BUCKET_ID = 1;
  static uint8_t const PEERS     = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &item)
  {
    auto map = map_constructor(2);

    map.Append(BUCKET_ID, item.bucket_id);
    map.Append(PEERS, item.peers);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &item)
  {
    map.ExpectKeyGetValue(BUCKET_ID, item.bucket_id);
    map.ExpectKeyGetValue(PEERS, item.peers);
  }
};
}  // namespace serializers
}  // namespace fetch
