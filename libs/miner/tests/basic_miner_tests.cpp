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

#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "meta/is_log2.hpp"
#include "miner/basic_miner.hpp"
#include "miner/resource_mapper.hpp"
#include "vectorise/platform.hpp"

#include <chrono>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <random>

using fetch::meta::IsLog2;
using fetch::meta::Log2;

class BasicMinerTests : public ::testing::TestWithParam<std::size_t>
{
protected:
  static constexpr uint32_t    NUM_LANES      = 64;
  static constexpr std::size_t NUM_SLICES     = 1024;
  static constexpr uint32_t    NUM_LANES_MASK = NUM_LANES - 1;
  static constexpr uint32_t    LOG2_NUM_LANES = Log2<NUM_LANES>::value;
  static constexpr std::size_t RANDOM_SEED    = 42;

  static_assert(IsLog2<NUM_LANES>::value, "Number of lanes must be a valid 2 power");

  using Rng                 = std::mt19937_64;
  using BasicMiner          = fetch::miner::BasicMiner;
  using BasicMinerPtr       = std::unique_ptr<BasicMiner>;
  using MutableTransaction  = fetch::ledger::MutableTransaction;
  using VerifiedTransaction = fetch::ledger::VerifiedTransaction;
  using Clock               = std::chrono::high_resolution_clock;
  using Timepoint           = Clock::time_point;
  using BitVector           = fetch::bitmanip::BitVector;
  using TransactionSummary  = fetch::ledger::TransactionSummary;
  using MainChain           = fetch::ledger::MainChain;
  using Block               = fetch::ledger::Block;

  void SetUp() override
  {
    rng_.seed(RANDOM_SEED);
    miner_ = std::make_unique<BasicMiner>(uint32_t{LOG2_NUM_LANES}, std::size_t{NUM_SLICES});
  }

  void TearDown() override
  {
    miner_.reset();
  }

  void PopulateWithTransactions(std::size_t num_transactions, size_t duplicates = 1)
  {
    std::poisson_distribution<uint32_t> dist(5.0);

    for (std::size_t i = 0; i < num_transactions; ++i)
    {
      uint32_t const num_resources = dist(rng_);

      MutableTransaction transaction;
      transaction.set_fee(rng_() & 0x3f);
      transaction.set_contract_name("ai.fetch.dummy");

      // Guarantee all TXs unique
      transaction.PushResource("Unique: " + std::to_string(i));

      for (std::size_t j = 0; j < num_resources; ++j)
      {
        transaction.PushResource("Resource: " + std::to_string(rng_()));
      }

      // convert the transaction to a valid transaction
      auto tx = VerifiedTransaction::Create(std::move(transaction));

      for (std::size_t i = 0; i < duplicates; ++i)
      {
        miner_->EnqueueTransaction(tx.summary());
      }
    }
  }

  Rng           rng_;
  BasicMinerPtr miner_;
};

TEST_P(BasicMinerTests, SimpleExample)
{
  std::size_t const num_tx = GetParam();

  PopulateWithTransactions(num_tx);

  Block     block;
  MainChain dummy{MainChain::Mode::IN_MEMORY_DB};

  block.body.previous_hash = dummy.GetHeaviestBlockHash();

  miner_->GenerateBlock(block, NUM_LANES, NUM_SLICES, dummy);

  for (auto const &slice : block.body.slices)
  {
    BitVector lanes{NUM_LANES};

    for (auto const &tx : slice)
    {
      BitVector resources{NUM_LANES};

      // update the resources array with the correct bits flags for the lanes
      for (auto const &resource : tx.resources)
      {
        // map the resource to a lane
        uint32_t const lane =
            fetch::miner::MapResourceToLane(resource, tx.contract_name, LOG2_NUM_LANES);

        // check the lane mapping
        resources.set(lane, 1);
      }

      // ensure there are not collisions
      BitVector collisions = resources & lanes;
      EXPECT_EQ(0, collisions.PopCount());

      lanes |= resources;
    }
  }
}

TEST_P(BasicMinerTests, reject_replayed_transactions)
{
  std::size_t num_tx = GetParam();
  // choose manual lanes + slices as we want to generate a lot of blocks to thoroughly test the main
  // chain
  std::size_t lanes  = 16;
  std::size_t slices = 16;

  miner_->log2_num_lanes() = uint32_t(fetch::platform::ToLog2(uint32_t(lanes)));

  PopulateWithTransactions(num_tx, 1);
  MainChain                    chain{MainChain::Mode::IN_MEMORY_DB};
  std::set<TransactionSummary> transactions_already_seen;
  std::set<TransactionSummary> transactions_within_block;

  while (miner_->GetBacklog() > 0)
  {
    Block block;

    block.body.previous_hash = chain.GetHeaviestBlockHash();

    miner_->GenerateBlock(block, lanes, slices, chain);

    // Check no duplicate transactions within a block
    transactions_within_block.clear();
    for (auto const &slice : block.body.slices)
    {
      for (auto const &tx : slice)
      {
        // Guarantee each body fresh transactions
        bool not_found = transactions_within_block.find(tx) == transactions_within_block.end();
        EXPECT_EQ(not_found, true);
        transactions_within_block.insert(tx);
      }
    }

    // check that transactions_within_block not inside transactions_already_seen (TX already in main
    // chain)
    for (auto const &tx : transactions_within_block)
    {
      // Guarantee each body fresh transactions
      bool not_found = transactions_already_seen.find(tx) == transactions_already_seen.end();
      EXPECT_EQ(not_found, true);
      transactions_already_seen.insert(tx);
    }

    block.UpdateDigest();

    /* Note no mining needed here - main chain doesn't care */
    chain.AddBlock(block);
  }

  // Now, push on all of the TXs that are already in the blockchain
  for (auto const &tx_summary : transactions_already_seen)
  {
    miner_->EnqueueTransaction(tx_summary);
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
        bool not_found = transactions_within_block.find(tx) == transactions_within_block.end();
        EXPECT_EQ(not_found, true);
        transactions_within_block.insert(tx);
      }
    }

    // check that transactions_within_block not inside transactions_already_seen (TX already in main
    // chain)
    for (auto const &tx : transactions_within_block)
    {
      // Guarantee each body fresh transactions
      bool not_found = transactions_already_seen.find(tx) == transactions_already_seen.end();
      EXPECT_EQ(not_found, true);
      transactions_already_seen.insert(tx);
    }

    block.UpdateDigest();

    /* Note no mining needed here - main chain doesn't care */
    chain.AddBlock(block);
  }
}

INSTANTIATE_TEST_CASE_P(ParamBased, BasicMinerTests, ::testing::Values(10, 20), );
