#pragma once
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

#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "ledger/chain/main_chain.hpp"
#include "miner/miner_interface.hpp"

#include <chrono>
#include <random>
#include <thread>

namespace fetch {
namespace chain {

// TODO(issue 33): fine for now, but it would be more efficient if the block
// coordinator launched mining tasks
class MainChainMiner
{
public:
  using BlockType             = chain::MainChain::BlockType;
  using BlockHash             = chain::MainChain::BlockHash;
  using BodyType              = chain::MainChain::BlockType::body_type;
  using Miner                 = fetch::chain::consensus::DummyMiner;
  using MinerInterface        = fetch::miner::MinerInterface;
  using BlockCompleteCallback = std::function<void(BlockType const &)>;

  static constexpr char const *LOGGING_NAME = "MainChainMiner";

  MainChainMiner(std::size_t num_lanes, std::size_t num_slices, chain::MainChain &mainChain,
                 chain::BlockCoordinator &blockCoordinator, MinerInterface &miner,
                 uint64_t minerNumber)
    : num_lanes_{num_lanes}
    , num_slices_{num_slices}
    , mainChain_{mainChain}
    , blockCoordinator_{blockCoordinator}
    , miner_{miner}
    , minerNumber_{minerNumber}
  {}

  ~MainChainMiner()
  {
    Stop();
  }

  void Start()
  {
    stop_   = false;
    thread_ = std::thread{&MainChainMiner::MinerThreadEntrypoint, this};
  }

  void Stop()
  {
    stop_ = true;
    if (thread_.joinable())
    {
      thread_.join();
    }
  }

  void OnBlockComplete(BlockCompleteCallback const &func)
  {
    on_block_complete_ = func;
  }

private:
  static constexpr uint32_t MAX_BLOCK_JITTER_US = 8000;
  static constexpr uint32_t BLOCK_PERIOD_MS     = 15000;

  using Clock     = std::chrono::high_resolution_clock;
  using Timestamp = Clock::time_point;

  template <typename T>
  Timestamp CalculateNextBlockTime(T &rng)
  {
    Timestamp block_time = Clock::now() + std::chrono::milliseconds{uint32_t{BLOCK_PERIOD_MS}};
    block_time += std::chrono::microseconds{rng() % MAX_BLOCK_JITTER_US};

    return block_time;
  }

  void MinerThreadEntrypoint()
  {
    std::random_device rd;
    std::mt19937       rng(rd());

    // schedule the next block time
    Timestamp next_block_time = CalculateNextBlockTime(rng);

    BlockHash previous_heaviest;

    BlockType next_block;
    BodyType  next_block_body;

    bool searching_for_hash = false;

    while (!stop_)
    {
      // determine the heaviest block
      auto &block = mainChain_.HeaviestBlock();

      // if the heaviest block has changed then we need to schedule the next block time
      if (block.hash() != previous_heaviest)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "==> New heaviest block: ", byte_array::ToBase64(block.hash()),
                       " from: ", byte_array::ToBase64(block.prev()));

        // new heaviest has been detected
        next_block_time    = CalculateNextBlockTime(rng);
        previous_heaviest  = block.hash().Copy();
        searching_for_hash = false;
      }

      if (searching_for_hash)
      {
        if (Miner::Mine(next_block, 100))
        {
          // Add the block
          blockCoordinator_.AddBlock(next_block);

          // TODO(EJF): Feels like this needs to be reworked into the block coordinator
          if (on_block_complete_)
          {
            on_block_complete_(next_block);
          }

          // stop searching for the hash and schedule the next time to generate a block
          next_block_time    = CalculateNextBlockTime(rng);
          searching_for_hash = false;
        }
      }
      else if (Clock::now() >= next_block_time)  // if we are ready to generate a new block
      {
        // update the metadata for the block
        next_block_body.block_number  = block.body().block_number + 1;
        next_block_body.previous_hash = block.hash();
        next_block_body.miner_number  = minerNumber_;

        FETCH_LOG_INFO(LOGGING_NAME, "Generate new block: ", num_lanes_, " x ", num_slices_);

        // Pack the block with transactions
        miner_.GenerateBlock(next_block_body, num_lanes_, num_slices_);
        next_block.SetBody(next_block_body);
        next_block.UpdateDigest();

        // Mine the block
        next_block.proof().SetTarget(target_);
        searching_for_hash = true;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
  }

  std::atomic<bool> stop_{false};
  std::size_t       target_ = 8;
  std::size_t       num_lanes_;
  std::size_t       num_slices_;

  chain::MainChain &       mainChain_;
  chain::BlockCoordinator &blockCoordinator_;
  MinerInterface &         miner_;
  std::thread              thread_;
  uint64_t                 minerNumber_{0};
  BlockCompleteCallback    on_block_complete_;
};

}  // namespace chain
}  // namespace fetch
