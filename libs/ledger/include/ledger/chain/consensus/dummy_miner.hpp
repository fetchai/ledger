#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

namespace fetch {
namespace chain {
namespace consensus {

class DummyMiner
{

public:
  // Blocking mine
  template <typename block_type>
  static void Mine(block_type &block)
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
  template <typename block_type>
  static bool Mine(block_type &block, uint64_t iterations)
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
    static std::random_device                      rd;
    static std::mt19937                            gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(0, std::numeric_limits<uint32_t>::max());
    return dis(gen);
  }
};
}  // namespace consensus
}  // namespace chain
}  // namespace fetch
