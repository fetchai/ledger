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

#include "ledger/chain/constants.hpp"
#include "ledger/execution_manager.hpp"
#include "storage/resource_mapper.hpp"

#include <algorithm>
#include <random>

struct TestBlock
{
  using ResourceIdMap = std::vector<std::string>;
  using BlockBody     = fetch::ledger::Block::Body;
  using Digest        = fetch::ledger::v2::Digest;

  static constexpr char const *LOGGING_NAME = "TestBlock";

  static constexpr uint64_t    IV          = uint64_t(-1);
  static constexpr std::size_t HASH_LENGTH = 32;

  BlockBody block;
  int       num_transactions = 0;

  template <typename RNG>
  static Digest GenerateHash(RNG &rng)
  {
    static constexpr std::size_t HASH_SIZE = 32;
    fetch::byte_array::ByteArray digest;
    digest.Resize(HASH_SIZE);
    for (std::size_t i = 0; i < HASH_SIZE; ++i)
    {
      digest[i] = static_cast<uint8_t>(rng() & 0xFF);
    }
    return {digest};
  }

  void GenerateBlock(uint32_t seed, uint32_t log2_num_lanes, std::size_t num_slices,
                     Digest const &previous_hash)
  {
    std::mt19937 rng;
    rng.seed(seed);

    ResourceIdMap     resources = BuildResourceMap(log2_num_lanes);
    std::size_t const num_lanes = 1u << log2_num_lanes;

    FETCH_LOG_DEBUG(LOGGING_NAME, "Generating block: ", num_lanes, " x ", num_slices);

    // generate the block hash and assign the previous hash
    fetch::byte_array::ByteArray digest;
    digest.Resize(std::size_t(HASH_LENGTH));
    for (std::size_t i = 0; i < std::size_t(HASH_LENGTH); ++i)
    {
      digest[i] = static_cast<uint8_t>(rng() & 0xFF);
    }
    block.hash          = digest;
    block.previous_hash = previous_hash;

    // generate a series of transactions to be populated in the block.
    // Generation put into loop to catch the rare case when on a single loop no
    // transactions are generated.
    while (num_transactions == 0)
    {
      // reset
      num_transactions = 0;
      block.slices.clear();
      block.slices.resize(num_slices);

      // main generation loop - iterate over all of the slices
      for (std::size_t slice = 0; slice < num_slices; ++slice)
      {
        auto &current_slice = block.slices[slice];

        std::size_t remaining_lanes = num_lanes;
        std::size_t lane_offset     = 0;

        while (remaining_lanes)
        {

          // decide how many lanes will be consumed this round
          std::size_t const consumed_lanes =
              std::min(std::max<std::size_t>(rng() % remaining_lanes, 1u), remaining_lanes);
          // decide the transaction type
          bool is_empty = (rng() & 0xFF) < 25;  // ~10%

          if (!is_empty)
          {
            fetch::BitVector mask{num_lanes};
            for (std::size_t i = 0; i < consumed_lanes; ++i)
            {
              mask.set(i, 1);
            }

            // create the transaction summary
            current_slice.emplace_back(
                fetch::ledger::v2::TransactionLayout{GenerateHash(rng), mask, 1, 0, 100});

            ++num_transactions;
          }

          // update lane indexes
          lane_offset += consumed_lanes;
          remaining_lanes -= consumed_lanes;
        }
      }
    }
  }

  ResourceIdMap BuildResourceMap(uint32_t log2_num_lanes)
  {
    //#define PROFILE_GENERATION_TIME

#ifdef PROFILE_GENERATION_TIME
    using Clock     = std::chrono::high_resolution_clock;
    using Timepoint = Clock::time_point;
    using Duration  = Clock::duration;

    Timepoint const start = Clock::now();
#endif  // PROFILE_GENERATION_TIME

    uint32_t const num_lanes = 1u << log2_num_lanes;

    ResourceIdMap                values{num_lanes};
    std::unordered_set<uint32_t> set;
    for (uint32_t i = 0; i < num_lanes; ++i)
    {
      set.insert(i);
    }

    std::size_t index = 0;
    while (!set.empty())
    {

      // create a value
      std::ostringstream oss;
      oss << "Resource: " << index++;
      std::string const prefix = "fetch.dummy.state.";
      std::string const value  = oss.str();

      // create the resource
      uint32_t const resource =
          fetch::storage::ResourceAddress{prefix + value}.lane(log2_num_lanes);

      // locate the resource
      auto it = set.find(resource);
      if (it != set.end())
      {
        values[resource] = value;
        set.erase(it);
      }
    }

#ifdef PROFILE_GENERATION_TIME
    Duration const elapsed_time = Clock::now() - start;

    std::cerr << "Generation Time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count() << "ms"
              << std::endl;
#endif  // PROFILE_GENERATION_TIME

    return values;
  }

  static TestBlock Generate(std::size_t log2_num_lanes, std::size_t num_slices, uint32_t seed,
                            Digest const &previous_hash = fetch::ledger::GENESIS_DIGEST)
  {
    TestBlock block;
    block.GenerateBlock(seed, static_cast<uint32_t>(log2_num_lanes), num_slices, previous_hash);
    return block;
  }
};
