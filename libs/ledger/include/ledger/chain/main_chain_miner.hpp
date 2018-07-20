#ifndef MAIN_CHAIN_MINER_HPP
#define MAIN_CHAIN_MINER_HPP

#include <thread>

#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"

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
  typedef fetch::chain::consensus::DummyMiner     miner;

  MainChainMiner(chain::MainChain &mainChain, chain::BlockCoordinator &blockCoordinator) :
    mainChain_{mainChain},
    blockCoordinator_{blockCoordinator}
  {
  }

  ~MainChainMiner()
  {
    Stop();
  }

  void start()
  {
    stop_ = false;

    auto closure = [this]
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

        nextBlock.SetBody(nextBody);
        nextBlock.UpdateDigest();

        // Mine the block
        nextBlock.proof().SetTarget(target_);
        miner::Mine(nextBlock);

        // Add the block
        blockCoordinator_.AddBlock(nextBlock);

        std::this_thread::sleep_for(std::chrono::seconds{30});
      }
    };

    thread_ =  std::thread{closure};
  }

  void Stop()
  {
    if(thread_.joinable())
    {
      stop_ = true;
      thread_.join();
    }
  }

private:

  bool                           stop_{false};
  uint64_t                       minerNumber_{0};
  std::size_t                    target_ = 15;
  chain::MainChain               &mainChain_;
  chain::BlockCoordinator        &blockCoordinator_;
  std::thread                    thread_;

};

}
}

#endif
