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

#include "beacon/beacon_manager.hpp"
#include "core/byte_array/const_byte_array.hpp"
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
  using Identity  = crypto::Identity;
  using TimeStamp = std::chrono::time_point<std::chrono::system_clock>;

  std::set<Identity> members{};
  uint64_t           round_start{0};
  uint64_t           round_end{0};

  // Timeouts for waiting for other members
  uint64_t start_reference_timepoint{uint64_t(-1)};
};

struct AeonExecutionUnit
{
  using BeaconManager  = dkg::BeaconManager;
  using ConstByteArray = byte_array::ConstByteArray;
  using SignatureShare = dkg::BeaconManager::SignedMessage;

  BeaconManager  manager{};
  SignatureShare member_share;

  Aeon aeon;

  bool observe_only{false};
};

}  // namespace beacon
}  // namespace fetch
