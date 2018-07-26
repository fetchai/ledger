#pragma once

#include "ledger/execution_manager.hpp"
#include "storage/resource_mapper.hpp"

#include <algorithm>
#include <random>

struct TestBlock
{
  using resource_id_map_type = std::vector<std::string>;
  using block_type           = fetch::ledger::ExecutionManager::block_type;
  using block_digest_type = fetch::ledger::ExecutionManager::block_digest_type;

  static constexpr uint64_t    IV          = uint64_t(-1);
  static constexpr std::size_t HASH_LENGTH = 32;

  block_type block;
  int        num_transactions = 0;

  template <typename RNG>
  static block_digest_type GenerateHash(RNG &rng)
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

  void GenerateBlock(uint32_t seed, uint32_t log2_num_lanes,
                     std::size_t              num_slices,
                     block_digest_type const &previous_hash)
  {

    std::mt19937 rng;
    rng.seed(seed);

    resource_id_map_type resources = BuildResourceMap(log2_num_lanes);
    std::size_t const    num_lanes = 1u << log2_num_lanes;

    fetch::logger.Info("Generating block: ", num_lanes, " x ", num_slices);

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
              std::min(std::max<std::size_t>(rng() % remaining_lanes, 1u),
                       remaining_lanes);
          // decide the transaction type
          bool is_empty = (rng() & 0xFF) < 25;  // ~10%

          if (!is_empty)
          {

            // create the transaction summary
            fetch::chain::TransactionSummary summary;
            summary.transaction_hash = GenerateHash(rng);
            summary.contract_name_   = "fetch.dummy.run";

            //            fetch::logger.Info("Generating TX: ",
            //            fetch::byte_array::ToBase64(summary.transaction_hash));

            // update the groups
            for (std::size_t i = 0; i < consumed_lanes; ++i)
            {
              std::size_t const index = (i + lane_offset);

              //              fetch::logger.Info(" - Resource: ", index);

              summary.resources.insert(resources.at(index));
            }

            current_slice.transactions.emplace_back(std::move(summary));
            ++num_transactions;
          }

          // update lane indexes
          lane_offset += consumed_lanes;
          remaining_lanes -= consumed_lanes;
        }
      }
    }
  }

  resource_id_map_type BuildResourceMap(uint32_t log2_num_lanes)
  {
    uint32_t const num_lanes = 1u << log2_num_lanes;

    resource_id_map_type         values{num_lanes};
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
          fetch::storage::ResourceID{prefix + value}.lane(log2_num_lanes);

      // locate the resource
      auto it = set.find(resource);
      if (it != set.end())
      {
        values[resource] = value;
        set.erase(it);
      }
    }

    return values;
  }

  static TestBlock Generate(
      std::size_t log2_num_lanes, std::size_t num_slices, uint32_t seed,
      block_digest_type const &previous_hash = block_digest_type{})
  {
    TestBlock block;
    block.GenerateBlock(seed, static_cast<uint32_t>(log2_num_lanes), num_slices,
                        previous_hash);
    return block;
  }
};

