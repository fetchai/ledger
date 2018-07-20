#ifndef MAIN_CHAIN_MINER_HPP
#define MAIN_CHAIN_MINER_HPP

#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "miner/miner_interface.hpp"

#include <thread>
#include <random>

namespace fetch
{
namespace chain
{

// TODO: (`HUT`) : fine for now, but it would be more efficient if the block coordinator launched
// mining tasks
class MainChainMiner
{
public:

  typedef chain::MainChain::block_type            block_type;
  typedef chain::MainChain::block_hash            block_hash;
  typedef chain::MainChain::block_type::body_type body_type;
  typedef fetch::chain::consensus::DummyMiner     dummy_miner_type;
  typedef fetch::miner::MinerInterface            miner_type;

  MainChainMiner(std::size_t num_lanes,
                 std::size_t num_slices,
                 chain::MainChain &mainChain,
                 chain::BlockCoordinator &blockCoordinator,
                 miner_type &miner)
    : num_lanes_{num_lanes}
    , num_slices_{num_slices}
    , mainChain_{mainChain}
    , blockCoordinator_{blockCoordinator}
    , miner_{miner}
  {
  }

  ~MainChainMiner()
  {
    Stop();
  }

  void start()
  {
    stop_ = false;
    thread_ = std::thread{&MainChainMiner::MinerThreadEntrypoint, this};
  }

  void Stop()
  {
    stop_ = true;
    if(thread_.joinable())
    {
      thread_.join();
    }
  }

private:

  void MinerThreadEntrypoint()
  {
    while(!stop_)
    {
      // Get heaviest block
      auto &block = mainChain_.HeaviestBlock();

      // Create another block sequential to previous
      block_type nextBlock;
      body_type nextBody;
      nextBody.block_number = block.body().block_number + 1;
      nextBody.previous_hash = block.hash();
      nextBody.miner_number = minerNumber_;

      // Pack the block with transactions
      miner_.GenerateBlock(nextBody, num_lanes_, num_slices_);
      nextBlock.SetBody(nextBody);
      nextBlock.UpdateDigest();

      // Mine the block
      nextBlock.proof().SetTarget(target_);
      dummy_miner_type::Mine(nextBlock);

      // Add the block
      blockCoordinator_.AddBlock(nextBlock);

      std::this_thread::sleep_for(std::chrono::seconds{5});
    }
  }

  std::atomic<bool>              stop_{false};
  uint64_t                       minerNumber_{0};
  std::size_t                    target_ = 15;
  std::size_t                    num_lanes_;
  std::size_t                    num_slices_;

  chain::MainChain               &mainChain_;
  chain::BlockCoordinator        &blockCoordinator_;
  miner_type                     &miner_;
  std::thread                    thread_;
};

}
}

#endif
