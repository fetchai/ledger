#ifndef FETCH_TEST_BLOCK_HPP
#define FETCH_TEST_BLOCK_HPP

#include "ledger/execution_manager.hpp"

#include <algorithm>
#include <random>

struct TestBlock {
  using block_digest_type = fetch::ledger::ExecutionManager::block_digest_type;
  using block_map_type = fetch::ledger::ExecutionManager::block_map_type;
  using tx_index_type = fetch::ledger::ExecutionManager::tx_index_type;

  static constexpr uint64_t IV = uint64_t(-1);
  static constexpr std::size_t HASH_LENGTH = 32;

  TestBlock(std::size_t lanes, std::size_t slices)
    : num_lanes(lanes), num_slices(slices), map(num_slices * num_lanes, uint64_t(IV))
  {}

  block_digest_type hash;
  std::size_t const num_lanes;
  std::size_t const num_slices;
  block_map_type map;
  tx_index_type index;

  bool IsWellFormed() const {
    std::size_t const largest_tx_id = static_cast<std::size_t>(
      std::accumulate(map.begin(), map.end(), 0, [](uint64_t a, uint64_t b) {
        bool const a_valid = (uint64_t(IV) != a);
        bool const b_valid = (uint64_t(IV) != b);

        if (a_valid && b_valid) {
          return std::max(a, b);
        } else if (a_valid) {
          return a;
        } else if (b_valid) {
          return b;
        } else {
          return IV;
        }
      })
    );

    return (largest_tx_id + 1u) == index.size();
  }

  void Generate(uint32_t seed = 42)
  {
    std::mt19937 rng;
    rng.seed(seed);

    // generate the hash
    fetch::byte_array::ByteArray digest;
    digest.Resize(std::size_t(HASH_LENGTH));
    for (std::size_t i = 0; i < std::size_t(HASH_LENGTH); ++i) {
      digest[i] = (rng() & 0xFF);
    }

    uint64_t tx_idx = 0;
    for (std::size_t slice = 0; slice < num_slices; ++slice)
    {
      std::size_t remaining_lanes = num_lanes;
      std::size_t lane_offset = 0;

      while (remaining_lanes)
      {
        // decide how many lanes will be consumed this round
        std::size_t const consumed_lanes = std::min(std::max<std::size_t>(rng() % remaining_lanes, 1u),
                                                    remaining_lanes);
        // decide the transaction type
        bool is_empty = (rng() & 0xFF) < 25; // ~10%

        for (std::size_t i = 0; i < consumed_lanes; ++i)
        {
          std::size_t const index = ((i + lane_offset) * num_slices) + slice;
          map.at(index) = (is_empty) ? uint64_t(IV) : tx_idx;
        }

        // increment the transaction index
        if (!is_empty)
          ++tx_idx;

        // update lane indexes
        lane_offset += consumed_lanes;
        remaining_lanes -= consumed_lanes;
      }
    }

    // create the index
    for (std::size_t tx = 0; tx < tx_idx; ++tx)
    {
      index.emplace_back(std::to_string(rng()) + "transaction " + std::to_string(tx));
    }
  }

  friend std::ostream &operator<<(std::ostream &stream, TestBlock const &block)
  {
    for (std::size_t lane = 0; lane < block.num_lanes; ++lane)
    {
      for (std::size_t slice = 0; slice < block.num_slices; ++slice)
      {
        uint64_t const element = block.map.at(lane * block.num_slices + slice);

        if (slice == 0)
          stream << "| ";

        if (element == uint64_t(IV))
        {
          stream << " - ";
        }
        else
        {
          stream << std::setfill(' ') << std::setw(3) << element;
        }

        stream << " |";
      }
      stream << '\n';
    }

    return stream;
  }

  static TestBlock Generate(std::size_t lanes, std::size_t slices)
  {
    TestBlock block(lanes, slices);
    block.Generate();
    return block;
  }
};

#endif //FETCH_TEST_BLOCK_HPP
