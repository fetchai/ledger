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

#include <random>

#include "ledger/chain/consensus/consensus_miner_interface.hpp"

namespace fetch {
namespace chain {
namespace consensus {

class DummyMiner : public ConsensusMinerInterface
{

public:
  DummyMiner()  = default;
  ~DummyMiner() = default;

  // Blocking mine
  void Mine(BlockType &block) override
  {
    uint64_t initNonce = GetRandom();
    block.body().nonce = initNonce;

    block.UpdateDigest();

    while (!block.proof()())
    {
      block.body().nonce++;
      block.UpdateDigest();
    }
  }

  // Mine for set number of iterations
  bool Mine(BlockType &block, uint64_t iterations) override
  {
    uint32_t initNonce = GetRandom();
    block.body().nonce = initNonce;

    block.UpdateDigest();

    while (!block.proof()() && iterations > 0)
    {
      block.body().nonce++;
      block.UpdateDigest();
      iterations--;
    }

    return block.proof()();
  }

private:
  static uint32_t GetRandom()
  {
    std::random_device                      rd;
    std::mt19937                            gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());
    return dis(gen);
  }
};
}  // namespace consensus
}  // namespace chain
}  // namespace fetch
