#pragma once

#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "ledger/chain/main_chain.hpp"
#include "miner/miner_interface.hpp"

#include <chrono>
#include <random>
#include <thread>

namespace fetch {
namespace chain {

// TODO: (`HUT`) : fine for now, but it would be more efficient if the block
// coordinator launched mining tasks
class MainChainMiner
{
public:
  using block_type       = chain::MainChain::block_type;
  using block_hash       = chain::MainChain::block_hash;
  using body_type        = chain::MainChain::block_type::body_type;
  using dummy_miner_type = fetch::chain::consensus::DummyMiner;
  using miner_type       = fetch::miner::MinerInterface;

  MainChainMiner(std::size_t num_lanes, std::size_t num_slices, chain::MainChain &mainChain,
                 chain::BlockCoordinator &blockCoordinator, miner_type &miner, uint64_t minerNumber)
    : num_lanes_{num_lanes}
    , num_slices_{num_slices}
    , mainChain_{mainChain}
    , blockCoordinator_{blockCoordinator}
    , miner_{miner}
    , minerNumber_{minerNumber}
  {}

  ~MainChainMiner() { Stop(); }

  void start()
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

  void onBlockComplete(std::function<void (const block_type)> func)
  {
    onBlockComplete_ = func;
  }

private:
  using clock_type     = std::chrono::high_resolution_clock;
  using timestamp_type = clock_type::time_point;

  std::function<void (const block_type)> onBlockComplete_;

  template <typename T>
  timestamp_type CalculateNextBlockTime(T &rng)
  {
    static constexpr uint32_t MAX_BLOCK_JITTER_US = 8000;
    static constexpr uint32_t BLOCK_PERIOD_MS     = 6000;

    timestamp_type block_time = clock_type::now() + std::chrono::milliseconds{BLOCK_PERIOD_MS};
    block_time += std::chrono::microseconds{rng() % MAX_BLOCK_JITTER_US};

    return block_time;
  }

  void MinerThreadEntrypoint()
  {
    std::random_device rd;
    std::mt19937       rng(rd());

    // schedule the next block time
    timestamp_type next_block_time = CalculateNextBlockTime(rng);

    block_hash previous_heaviest;

    while (!stop_)
    {
      if (clock_type::now() < next_block_time)
      {
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
        continue;
      }

      auto &block = mainChain_.HeaviestBlock();

      block_type nextBlock;
      body_type  nextBody;
      nextBody.block_number  = block.body().block_number + 1;
      nextBody.previous_hash = block.hash();
      nextBody.miner_number  = minerNumber_;

      // Pack the block with transactions
      miner_.GenerateBlock(nextBody, num_lanes_, num_slices_);
      nextBlock.SetBody(nextBody);
      nextBlock.UpdateDigest();

      // Mine the block
      nextBlock.proof().SetTarget(target_);
      if (dummy_miner_type::Mine(nextBlock, 100))
      {
        // Add the block
        blockCoordinator_.AddBlock(nextBlock);
        if (onBlockComplete_)
        {
          onBlockComplete_(nextBlock);
        }
        next_block_time = CalculateNextBlockTime(rng);
      }
    }
  }

  std::atomic<bool> stop_{false};
  std::size_t       target_ = 8;
  std::size_t       num_lanes_;
  std::size_t       num_slices_;

  chain::MainChain &       mainChain_;
  chain::BlockCoordinator &blockCoordinator_;
  miner_type &             miner_;
  std::thread              thread_;
  uint64_t                 minerNumber_{0};
};

}  // namespace chain
}  // namespace fetch
