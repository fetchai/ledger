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
#include "ledger/chain/consensus/consensus_miner_interface.hpp"
#include "ledger/chain/main_chain.hpp"
#include "metrics/metrics.hpp"
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
  using BlockType               = chain::MainChain::BlockType;
  using BlockHash               = chain::MainChain::BlockHash;
  using BodyType                = chain::MainChain::BlockType::body_type;
  using ConsensusMinerInterface = std::shared_ptr<fetch::chain::consensus::ConsensusMinerInterface>;
  using MinerInterface          = fetch::miner::MinerInterface;
  using BlockCompleteCallback   = std::function<void(BlockType const &)>;

  static constexpr char const *LOGGING_NAME    = "MainChainMiner";
  static constexpr uint32_t    BLOCK_PERIOD_MS = 5000;

  MainChainMiner(std::size_t num_lanes, std::size_t num_slices, chain::MainChain &mainChain,
                 chain::BlockCoordinator &blockCoordinator, MinerInterface &miner,
                 ConsensusMinerInterface &consensus_miner, uint64_t minerNumber,
                 std::chrono::steady_clock::duration block_interval =
                     std::chrono::milliseconds{BLOCK_PERIOD_MS})
    : num_lanes_{num_lanes}
    , num_slices_{num_slices}
    , mainChain_{mainChain}
    , blockCoordinator_{blockCoordinator}
    , miner_{miner}
    , consensus_miner_{consensus_miner}
    , minerNumber_{minerNumber}
    , block_interval_{block_interval}
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

  void SetConsensusMiner(ConsensusMinerInterface consensus_miner)
  {
    consensus_miner_ = consensus_miner;
  }

private:
  using Clock        = std::chrono::high_resolution_clock;
  using Timestamp    = Clock::time_point;
  using Milliseconds = std::chrono::milliseconds;

  std::function<void(const BlockType)> onBlockComplete_;

  void MinerThreadEntrypoint()
  {
    // schedule the next block time
    Timestamp next_block_time = Clock::now() + block_interval_;

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
        next_block_time    = Clock::now() + block_interval_;
        previous_heaviest  = block.hash().Copy();
        searching_for_hash = false;
      }

      if (searching_for_hash)
      {
        if (consensus_miner_->Mine(next_block, 100))
        {
          // Add the block
          blockCoordinator_.AddBlock(next_block);

          // TODO(EJF): Feels like this needs to be reworked into the block coordinator
          if (on_block_complete_)
          {
            on_block_complete_(next_block);

            FETCH_METRIC_BLOCK_GENERATED(next_block.hash());
          }

          // stop searching for the hash and schedule the next time to generate a block
          next_block_time    = Clock::now() + block_interval_;
          searching_for_hash = false;
        }
      }
      else if (Clock::now() >= next_block_time)  // if we are ready to generate a new block
      {
        // update the metadata for the block
        next_block_body.block_number  = block.body().block_number + 1;
        next_block_body.previous_hash = block.hash();
        next_block_body.miner_number  = minerNumber_;

        // Reset previous state
        next_block_body.slices.clear();

        FETCH_LOG_INFO(LOGGING_NAME, "Generate new block: ", num_lanes_, " x ", num_slices_);

        // Pack the block with transactions
        miner_.GenerateBlock(next_block_body, num_lanes_, num_slices_);
        next_block.SetBody(next_block_body);
        next_block.UpdateDigest();

#ifdef FETCH_ENABLE_METRICS
        metrics::Metrics::Timestamp const now = metrics::Metrics::Clock::now();

        for (auto const &slice : next_block_body.slices)
        {
          for (auto const &tx : slice.transactions)
          {
            FETCH_METRIC_TX_PACKED_EX(tx.transaction_hash, now);
          }
        }
#endif  // FETCH_ENABLE_METRICS

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

  chain::MainChain &                  mainChain_;
  chain::BlockCoordinator &           blockCoordinator_;
  MinerInterface &                    miner_;
  ConsensusMinerInterface             consensus_miner_;
  std::thread                         thread_;
  uint64_t                            minerNumber_{0};
  BlockCompleteCallback               on_block_complete_;
  std::chrono::steady_clock::duration block_interval_;
};

}  // namespace chain
}  // namespace fetch
