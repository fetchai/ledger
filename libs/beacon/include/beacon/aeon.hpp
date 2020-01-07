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

#include "beacon/beacon_manager.hpp"
#include "beacon/block_entropy.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/digest.hpp"
#include "core/serializers/main_serializer_definition.hpp"
#include "crypto/prover.hpp"

#include <atomic>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fetch {
namespace beacon {

struct Aeon
{
  using MuddleAddress = byte_array::ConstByteArray;
  using Identity      = crypto::Identity;
  using TimeStamp     = std::chrono::time_point<std::chrono::system_clock>;
  using BlockEntropy  = beacon::BlockEntropy;

  std::set<MuddleAddress> members{};
  uint64_t                round_start{0};
  uint64_t                round_end{0};
  BlockEntropy            block_entropy_previous;

  // Timeouts for waiting for other members
  uint64_t start_reference_timepoint{uint64_t(-1)};

  bool operator==(Aeon const &other) const
  {
    return ((members == other.members) && (round_start == other.round_start) &&
            (round_end == other.round_end));
  }
};

// TODO(HUT): merge these into just Aeon
struct AeonExecutionUnit
{
  using BeaconManager  = dkg::BeaconManager;
  using ConstByteArray = byte_array::ConstByteArray;
  using SignatureShare = dkg::BeaconManager::SignedMessage;
  using BlockEntropy   = beacon::BlockEntropy;

  BlockEntropy   block_entropy;
  BeaconManager  manager{};
  SignatureShare member_share;

  Aeon aeon;
};
}  // namespace beacon

namespace serializers {

template <typename D>
struct MapSerializer<beacon::Aeon, D>
{
public:
  using Type       = beacon::Aeon;
  using DriverType = D;

  static uint8_t const MEMBERS                   = 1;
  static uint8_t const ROUND_START               = 2;
  static uint8_t const ROUND_END                 = 3;
  static uint8_t const BLOCK_ENTROPY_PREVIOUS    = 4;
  static uint8_t const START_REFERENCE_TIMEPOINT = 5;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &item)
  {
    auto map = map_constructor(5);

    map.Append(MEMBERS, item.members);
    map.Append(ROUND_START, item.round_start);
    map.Append(ROUND_END, item.round_end);
    map.Append(BLOCK_ENTROPY_PREVIOUS, item.block_entropy_previous);
    map.Append(START_REFERENCE_TIMEPOINT, item.start_reference_timepoint);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &item)
  {
    map.ExpectKeyGetValue(MEMBERS, item.members);
    map.ExpectKeyGetValue(ROUND_START, item.round_start);
    map.ExpectKeyGetValue(ROUND_END, item.round_end);
    map.ExpectKeyGetValue(BLOCK_ENTROPY_PREVIOUS, item.block_entropy_previous);
    map.ExpectKeyGetValue(START_REFERENCE_TIMEPOINT, item.start_reference_timepoint);
  }
};

template <typename D>
struct MapSerializer<beacon::AeonExecutionUnit, D>
{
public:
  using Type       = beacon::AeonExecutionUnit;
  using DriverType = D;

  static uint8_t const BLOCK_ENTROPY = 1;
  static uint8_t const MANAGER       = 2;
  static uint8_t const MEMBER_SHARE  = 3;
  static uint8_t const AEON          = 4;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &item)
  {
    auto map = map_constructor(4);

    map.Append(BLOCK_ENTROPY, item.block_entropy);
    map.Append(MANAGER, item.manager);
    map.Append(MEMBER_SHARE, item.member_share);
    map.Append(AEON, item.aeon);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &item)
  {
    map.ExpectKeyGetValue(BLOCK_ENTROPY, item.block_entropy);
    map.ExpectKeyGetValue(MANAGER, item.manager);
    map.ExpectKeyGetValue(MEMBER_SHARE, item.member_share);
    map.ExpectKeyGetValue(AEON, item.aeon);
  }
};

}  // namespace serializers

}  // namespace fetch
