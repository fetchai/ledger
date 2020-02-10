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
#include "core/serialisers/main_serialiser_definition.hpp"
#include "core/serialisers/map_serialiser_boilerplate.hpp"
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

namespace serialisers {

template <typename D>
struct MapSerialiser<beacon::Aeon, D>
  : MapSerialiserBoilerplate<beacon::Aeon, D, serialiseD_STRUCT_FIELD(1, beacon::Aeon::members),
                             serialiseD_STRUCT_FIELD(2, beacon::Aeon::round_start),
                             serialiseD_STRUCT_FIELD(3, beacon::Aeon::round_end),
                             serialiseD_STRUCT_FIELD(4, beacon::Aeon::block_entropy_previous),
                             serialiseD_STRUCT_FIELD(5, beacon::Aeon::start_reference_timepoint)>
{
};

template <typename D>
struct MapSerialiser<beacon::AeonExecutionUnit, D>
  : MapSerialiserBoilerplate<beacon::AeonExecutionUnit, D,
                             serialiseD_STRUCT_FIELD(1, beacon::AeonExecutionUnit::block_entropy),
                             serialiseD_STRUCT_FIELD(2, beacon::AeonExecutionUnit::manager),
                             serialiseD_STRUCT_FIELD(3, beacon::AeonExecutionUnit::member_share),
                             serialiseD_STRUCT_FIELD(4, beacon::AeonExecutionUnit::aeon)>
{
};

}  // namespace serialisers

}  // namespace fetch
