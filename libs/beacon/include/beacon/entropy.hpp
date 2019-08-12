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

#include "beacon/beacon_round.hpp"
#include "core/byte_array/const_byte_array.hpp"

#include <cstdint>
#include <vector>

namespace fetch {
namespace beacon {

struct Entropy
{
  using ConstByteArray = byte_array::ConstByteArray;
  using SignatureShare = BeaconRoundDetails::SignatureShare;
  using GroupSignature = dkg::BeaconManager::Signature;

  uint64_t       round{0};
  ConstByteArray seed{"genesis"};
  ConstByteArray entropy{};

  GroupSignature signature;

  bool operator<(Entropy const &other) const
  {
    // Lower rounds come first, hence >
    return round > other.round;
  }
};

}  // namespace beacon

namespace serializers {

template <typename D>
struct MapSerializer<beacon::Entropy, D>
{
public:
  using Type       = beacon::Entropy;
  using DriverType = D;

  static uint8_t const ROUND     = 0;
  static uint8_t const SEED      = 1;
  static uint8_t const ENTROPY   = 2;
  static uint8_t const SIGNATURE = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &member)
  {
    auto map = map_constructor(4);

    map.Append(ROUND, member.round);
    map.Append(SEED, member.seed);
    map.Append(ENTROPY, member.entropy);
    map.Append(SIGNATURE, member.signature);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &member)
  {
    map.ExpectKeyGetValue(ROUND, member.round);
    map.ExpectKeyGetValue(SEED, member.seed);
    map.ExpectKeyGetValue(ENTROPY, member.entropy);
    map.ExpectKeyGetValue(SIGNATURE, member.signature);
  }
};

}  // namespace serializers
}  // namespace fetch