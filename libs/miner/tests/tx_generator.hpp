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

#include "core/byte_array/byte_array.hpp"
#include "core/random/lcg.hpp"
#include "ledger/chain/v2/digest.hpp"
#include "ledger/chain/v2/transaction_layout.hpp"

class TransactionGenerator
{
public:
  using TransactionLayout = fetch::ledger::TransactionLayout;

  explicit TransactionGenerator(uint32_t log2_num_lanes = 0)
    : log2_num_lanes_{log2_num_lanes}
  {}

  void Seed(std::size_t seed = 42)
  {
    rng_.Seed(seed);
  }

  TransactionLayout operator()(uint32_t num_resources)
  {
    auto const index = index_++;
    return {GenerateDigest(), GenerateResources(num_resources), (index + 1) * 2, 1, 1000};
  }

private:
  using Rng       = fetch::random::LinearCongruentialGenerator;
  using RngWord   = Rng::random_type;
  using Digest    = fetch::ledger::Digest;
  using ByteArray = fetch::byte_array::ByteArray;

  Digest GenerateDigest()
  {
    static constexpr std::size_t DIGEST_SIZE   = 32;
    static constexpr std::size_t RNG_WORD_SIZE = sizeof(RngWord);
    static constexpr std::size_t NUM_WORDS     = DIGEST_SIZE / RNG_WORD_SIZE;

    static_assert((DIGEST_SIZE % RNG_WORD_SIZE) == 0, "Digest must be a multiple of RNG word");

    ByteArray digest{};
    digest.Resize(DIGEST_SIZE);

    auto *raw = reinterpret_cast<RngWord *>(digest.pointer());
    for (std::size_t i = 0; i < NUM_WORDS; ++i)
    {
      raw[i] = rng_();
    }

    return {digest};
  }

  fetch::BitVector GenerateResources(uint32_t num_resources)
  {
    fetch::BitVector mask{num_lanes_};

    for (uint32_t i = 0; i < num_resources; ++i)
    {
      auto const resource = rng_() % num_lanes_;

      mask.set(resource, 1);
    }

    return mask;
  }

  uint32_t const log2_num_lanes_{0};
  uint32_t const num_lanes_{1u << log2_num_lanes_};
  Rng            rng_{};
  uint32_t       index_{0};
};
