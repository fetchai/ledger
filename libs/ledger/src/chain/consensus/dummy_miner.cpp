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

#include "ledger/chain/consensus/dummy_miner.hpp"

#include "core/byte_array/encoders.hpp"
#include "ledger/chain/block.hpp"

#include <random>

static uint32_t GetRandom()
{
  std::random_device                      rd;
  std::mt19937                            gen(rd());
  std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());
  return dis(gen);
}

static constexpr char const *LOGGING_NAME = "DummyMiner";

using fetch::byte_array::ToHex;  // NOLINT - Used for debugging

namespace fetch {
namespace ledger {
namespace consensus {

void DummyMiner::Mine(Block &block)
{
  for (;;)
  {
    if (Mine(block, std::numeric_limits<uint64_t>::max()))
    {
      break;
    }
  }
}

bool DummyMiner::Mine(Block &block, uint64_t iterations)
{
  block.nonce = GetRandom();

  block.UpdateDigest();

  while (!block.proof() && iterations > 0)
  {
    block.nonce++;
    block.UpdateDigest();
    iterations--;
  }

  bool const success = block.proof();
  if (success)
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Proof: Digest: ", ToHex(block.proof.digest()));
    FETCH_LOG_DEBUG(LOGGING_NAME, "Proof: Target: ", ToHex(block.proof.target()));
  }

  return success;
}

}  // namespace consensus
}  // namespace ledger
}  // namespace fetch
