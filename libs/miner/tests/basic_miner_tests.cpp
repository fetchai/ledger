//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/meta/is_log2.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "miner/basic_miner.hpp"
#include "miner/resource_mapper.hpp"

#include <chrono>
#include <random>
#include <memory>
#include <fstream>
#include <gtest/gtest.h>

using fetch::meta::IsLog2;
using fetch::meta::Log2;
using fetch::byte_array::ToBase64;

class BasicMinerTests : public ::testing::TestWithParam<std::size_t>
{
protected:

  static constexpr uint32_t NUM_LANES  = 64;
  static constexpr std::size_t NUM_SLICES = 1024;
  static constexpr uint32_t NUM_LANES_MASK = NUM_LANES - 1;
  static constexpr uint32_t LOG2_NUM_LANES = Log2<NUM_LANES>::value;
  static constexpr std::size_t RANDOM_SEED = 42;

  static_assert(IsLog2<NUM_LANES>::value, "Number of lanes must be a valid 2 power");

  using Rng = std::mt19937_64;
  using BasicMiner = fetch::miner::BasicMiner;
  using BasicMinerPtr = std::unique_ptr<BasicMiner>;
  using MutableTransaction = fetch::chain::MutableTransaction;
  using VerifiedTransaction = fetch::chain::VerifiedTransaction;
  using BlockBody = fetch::chain::BlockBody;
  using Clock = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;
  using BitVector = fetch::bitmanip::BitVector;

  void SetUp() override
  {
    rng_.seed(RANDOM_SEED);
    miner_ = std::make_unique<BasicMiner>(uint32_t{LOG2_NUM_LANES}, std::size_t{NUM_SLICES});
  }

  void TearDown() override
  {
    miner_.reset();
  }

  void PopulateWithTransactions(std::size_t num_transactions)
  {
    std::poisson_distribution<uint32_t> dist(5.0);

    for (std::size_t i = 0; i < num_transactions; ++i)
    {
      uint32_t const num_resources = dist(rng_);

      MutableTransaction transaction;
      transaction.set_fee(rng_() & 0x3f);
      transaction.set_contract_name("ai.fetch.dummy");

      for (std::size_t j = 0; j < num_resources; ++j)
      {
        transaction.PushResource("Resource: " + std::to_string(rng_()));
      }

      // convert the transaction to a valid transaction
      auto tx = VerifiedTransaction::Create(std::move(transaction));
      miner_->EnqueueTransaction(tx.summary());
    }
  }

  Rng rng_;
  BasicMinerPtr miner_;
};

TEST_P(BasicMinerTests, Sample)
{
  std::size_t const txs_per_thread = GetParam();

  Timepoint const pop = Clock::now();
  PopulateWithTransactions(50000);

  Timepoint const start = Clock::now();
  BlockBody block;
  miner_->SetTxPerThread(txs_per_thread);
  miner_->GenerateBlock(block, NUM_LANES, NUM_SLICES);
  Timepoint const stop = Clock::now();

  std::size_t num_tx = 0;
  std::size_t total_fee = 0;
  std::size_t occupancy = 0;
  std::size_t i = 0;
  for (auto const &slice : block.slices)
  {
    BitVector lanes{NUM_LANES};

    num_tx += slice.transactions.size();

//    std::cout << "Slice " << i << std::endl;

    for (auto const &tx : slice.transactions)
    {
      total_fee += tx.fee;

      BitVector resources{NUM_LANES};

      // update the resources array with the correct bits flags for the lanes
      for (auto const &resource : tx.resources)
      {
        // map the resource to a lane
        uint32_t const lane = fetch::miner::MapResourceToLane(resource, tx.contract_name, LOG2_NUM_LANES);

//        std::cout << lanes << " (lane: " << (lane + 1) << ") " << ToBase64(tx.transaction_hash) << std::endl;

        // check the lane mapping
        resources.set(lane, 1);
      }

      // ensure there are not collisions
      BitVector collisions = resources & lanes;
      EXPECT_EQ(0, collisions.PopCount());

      lanes |= resources;
    }

    occupancy += lanes.PopCount();

    ++i;
  }

#if 1

  using std::chrono::duration_cast;
  using std::chrono::nanoseconds;

  std::ofstream statistics_file("stats.csv", std::ios::out | std::ios::app);

  if (statistics_file.tellp() == 0)
  {
    statistics_file << "Threads,GenTime,PopTime,NumTx,Fee,Occupancy,Slices,Lanes" << std::endl;
  }
  std::cout << "File Position:" << statistics_file.tellp() << std::endl;

  statistics_file << txs_per_thread
                  << ',' << duration_cast<nanoseconds>(stop - start).count()
                  << ',' << duration_cast<nanoseconds>(start - pop).count()
                  << ',' << num_tx
                  << ',' << total_fee
                  << ',' << occupancy
                  << ',' << NUM_SLICES
                  << ',' << NUM_LANES
                  << std::endl;
#endif
}

INSTANTIATE_TEST_CASE_P(ParamBased,
                        BasicMinerTests,
                        ::testing::Values(1,2,3,4,5,6,7,8),);