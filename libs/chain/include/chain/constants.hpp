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

#include "core/digest.hpp"

#include <cstdint>

namespace fetch {
namespace chain {

constexpr uint64_t FINALITY_PERIOD = 10;

Digest GetGenesisDigest();
Digest GetGenesisMerkleRoot();
void   SetGenesisDigest(Digest const &digest);
void   SetGenesisMerkleRoot(Digest const &digest);
void   InitialiseTestConstants();

// consensus related
extern uint64_t STAKE_WARM_UP_PERIOD;
extern uint64_t STAKE_COOL_DOWN_PERIOD;

extern Digest const GENESIS_DIGEST_DEFAULT;
extern Digest const GENESIS_MERKLE_ROOT_DEFAULT;

static constexpr std::size_t HASH_SIZE = 32;
extern Digest const          ZERO_HASH;

}  // namespace chain
}  // namespace fetch
