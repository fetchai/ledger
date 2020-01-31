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
#include "core/serializers/map_serializer_boilerplate.hpp"
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
  : MapSerializerBoilerplate<beacon::Aeon, D, EXPECTED_KEY_MEMBER(1, beacon::Aeon::members),
                             EXPECTED_KEY_MEMBER(2, beacon::Aeon::round_start),
                             EXPECTED_KEY_MEMBER(3, beacon::Aeon::round_end),
                             EXPECTED_KEY_MEMBER(4, beacon::Aeon::block_entropy_previous),
                             EXPECTED_KEY_MEMBER(5, beacon::Aeon::start_reference_timepoint)>
{
};

template <typename D>
struct MapSerializer<beacon::AeonExecutionUnit, D>
  : MapSerializerBoilerplate<beacon::AeonExecutionUnit, D,
                             EXPECTED_KEY_MEMBER(1, beacon::AeonExecutionUnit::block_entropy),
                             EXPECTED_KEY_MEMBER(2, beacon::AeonExecutionUnit::manager),
                             EXPECTED_KEY_MEMBER(3, beacon::AeonExecutionUnit::member_share),
                             EXPECTED_KEY_MEMBER(4, beacon::AeonExecutionUnit::aeon)>
{
};

}  // namespace serializers

}  // namespace fetch
