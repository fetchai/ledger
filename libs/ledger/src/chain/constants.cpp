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

#include "core/byte_array/decoders.hpp"
#include "core/digest.hpp"
#include "ledger/chain/constants.hpp"

namespace fetch {
namespace ledger {

using byte_array::FromBase64;

uint64_t STAKE_WARM_UP_PERIOD   = 100;
uint64_t STAKE_COOL_DOWN_PERIOD = 100;

Digest GENESIS_DIGEST      = FromBase64("0+++++++++++++++++Genesis+++++++++++++++++0=");
Digest GENESIS_MERKLE_ROOT = FromBase64("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");

}  // namespace ledger
}  // namespace fetch
