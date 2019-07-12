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

#include "core/bloom_filter.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/transaction_layout.hpp"
#include "meta/log2.hpp"
#include "miner/basic_miner.hpp"
#include "miner/resource_mapper.hpp"
#include "tx_generator.hpp"
#include "vectorise/platform.hpp"

#include "gtest/gtest.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>

using fetch::meta::IsLog2;
using fetch::meta::Log2;
using fetch::BitVector;
using fetch::ledger::Digest;
using fetch::ledger::DigestSet;
using fetch::ledger::DigestMap;

class BasicMinerTests : public ::testing::TestWithParam<std::size_t>
{
protected:
  static constexpr uint32_t    NUM_LANES      = 16;
  static constexpr std::size_t NUM_SLICES     = 16;
  static constexpr uint32_t    NUM_LANES_MASK = NUM_LANES - 1;
  static constexpr uint32_t    LOG2_NUM_LANES = fetch::meta::Log2(NUM_LANES);
  static constexpr std::size_t RANDOM_SEED    = 42;

  static_assert(IsLog2(NUM_LANES), "Number of lanes must be a valid 2 power");

  using Rng               = std::mt19937_64;
  using BasicMiner        = fetch::miner::BasicMiner;
  using BasicMinerPtr     = std::unique_ptr<BasicMiner>;
  using Clock             = std::chrono::high_resolution_clock;
  using Timepoint         = Clock::time_point;
  using TransactionLayout = fetch::ledger::TransactionLayout;
  using MainChain         = fetch::ledger::MainChain;
  using Block             = fetch::ledger::Block;
  using LayoutMap         = DigestMap<TransactionLayout>;

  void SetUp() override
  {
    rng_.seed(RANDOM_SEED);
    generator_.Seed(RANDOM_SEED);
    miner_ = std::make_unique<BasicMiner>(uint32_t{LOG2_NUM_LANES});
  }

  void TearDown() override
  {
    miner_.reset();
  }

  LayoutMap PopulateWithTransactions(std::size_t num_transactions, std::size_t duplicates = 1)
  {
    LayoutMap layout{};

    std::poisson_distribution<uint32_t> dist(5.0);

    for (std::size_t i = 0; i < num_transactions; ++i)
    {
      uint32_t const num_resources = dist(rng_);

      // generate the transaction with a number of transactions
      auto tx = generator_(num_resources);

      for (std::size_t i = 0; i < duplicates; ++i)
      {
        miner_->EnqueueTransaction(tx);
      }

      // ensure that the generator has not produced any duplicates
      assert(layout.find(tx.digest()) == layout.end());
    }

    return layout;
  }

  Rng                  rng_{};
  TransactionGenerator generator_{LOG2_NUM_LANES};
  BasicMinerPtr        miner_;
};

TEST_P(BasicMinerTests, SimpleExample)
{
  std::size_t const num_tx = GetParam();

  PopulateWithTransactions(num_tx);

  Block     block;
  MainChain dummy{std::make_unique<fetch::NullBloomFilter>(), MainChain::Mode::IN_MEMORY_DB};

  block.body.previous_hash = dummy.GetHeaviestBlockHash();

  miner_->GenerateBlock(block, NUM_LANES, NUM_SLICES, dummy);

  for (auto const &slice : block.body.slices)
  {
    BitVector lanes{NUM_LANES};

    for (auto const &tx : slice)
    {
      auto const &mask = tx.mask();

      EXPECT_EQ(mask.size(), uint32_t{NUM_LANES});

      // ensure there are not collisions
      BitVector const collisions = mask & lanes;
      EXPECT_EQ(0, collisions.PopCount());

      lanes |= mask;
    }
  }
}

TEST_P(BasicMinerTests, RejectReplayedTransactions)
{
  std::size_t const num_tx = GetParam();

  auto const layout = PopulateWithTransactions(num_tx, 1);

  MainChain chain{std::make_unique<fetch::NullBloomFilter>(), MainChain::Mode::IN_MEMORY_DB};

  DigestSet transactions_already_seen{};
  DigestSet transactions_within_block{};

  while (miner_->GetBacklog() > 0)
  {
    Block block;

    block.body.previous_hash = chain.GetHeaviestBlockHash();

    miner_->GenerateBlock(block, NUM_LANES, NUM_SLICES, chain);

    // Check no duplicate transactions within a block
    transactions_within_block.clear();
    for (auto const &slice : block.body.slices)
    {
      for (auto const &tx : slice)
      {
        // Guarantee each body fresh transactions
        EXPECT_TRUE(transactions_within_block.find(tx.digest()) == transactions_within_block.end());

        transactions_within_block.insert(tx.digest());
      }
    }

    // check that transactions_within_block not inside transactions_already_seen (TX already in main
    // chain)
    for (auto const &tx : transactions_within_block)
    {
      // Guarantee each body fresh transactions
      EXPECT_TRUE(transactions_already_seen.find(tx) == transactions_already_seen.end());

      transactions_already_seen.insert(tx);
    }

    block.UpdateDigest();

    /* Note no mining needed here - main chain doesn't care */
    chain.AddBlock(block);
  }

  // Now, push on all of the TXs that are already in the blockchain
  for (auto const &digest : transactions_already_seen)
  {
    auto const it = layout.find(digest);
    ASSERT_TRUE(it != layout.end());

    miner_->EnqueueTransaction(it->second);
  }

  while (miner_->GetBacklog() > 0)
  {
    Block block;

    block.body.previous_hash = chain.GetHeaviestBlockHash();

    miner_->GenerateBlock(block, NUM_LANES, NUM_SLICES, chain);

    for (auto const &slice : block.body.slices)
    {
      EXPECT_EQ(slice.size(), 0);
    }

    // Check no duplicate transactions within a block
    transactions_within_block.clear();
    for (auto const &slice : block.body.slices)
    {
      for (auto const &tx : slice)
      {
        // Guarantee each body fresh transactions
        ASSERT_TRUE(transactions_within_block.find(tx.digest()) == transactions_within_block.end());

        transactions_within_block.insert(tx.digest());
      }
    }

    // check that transactions_within_block not inside transactions_already_seen (TX already in main
    // chain)
    for (auto const &tx : transactions_within_block)
    {
      // Guarantee each body fresh transactions
      ASSERT_TRUE(transactions_already_seen.find(tx) == transactions_already_seen.end());

      transactions_already_seen.insert(tx);
    }

    block.UpdateDigest();

    /* Note no mining needed here - main chain doesn't care */
    chain.AddBlock(block);
  }
}

INSTANTIATE_TEST_CASE_P(ParamBased, BasicMinerTests, ::testing::Values(10, 20), );
