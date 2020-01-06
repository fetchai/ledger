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

#include <cstdint>
#include <vector>

#include "crypto/identity.hpp"

namespace fetch {
namespace ledger {

struct StakeUpdateEvent
{
  using StakeAmount = uint64_t;
  using BlockIndex  = uint64_t;

  BlockIndex       block_index{0};
  crypto::Identity from{};
  StakeAmount      amount{0};
};

using StakeUpdateEvents = std::vector<StakeUpdateEvent>;

}  // namespace ledger
}  // namespace fetch
