#ifndef BLOCK_COORDINATOR_HPP
#define BLOCK_COORDINATOR_HPP

#include <thread>

#include "ledger/chain/main_chain.hpp"
#include "ledger/execution_manager.hpp"

namespace fetch
{
namespace chain
{

class BlockCoordinator
{
public:

  typedef chain::MainChain::block_type block_type;
  typedef chain::MainChain::block_hash block_hash;
  typedef fetch::mutex::Mutex          mutex_type;

  BlockCoordinator(chain::MainChain &mainChain,
      std::shared_ptr<ledger::ExecutionManager> executionManager) :
    mainChain_{mainChain},
    executionManager_{executionManager}
  {
  }

  ~BlockCoordinator()
  {
    Stop();
  }

  void AddBlock(block_type &block)
  {
    mainChain_.AddBlock(block);

    auto heaviestHash = mainChain_.HeaviestBlock().hash();

    if(block.hash() == heaviestHash)
    {
      std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
      heaviestHashes_.push_front(heaviestHash);
    }
  }

  void start()
  {
    stop_ = false;

    auto closure = [this]
    {
      while(!stop_)
      {
        block_hash heaviest;

        {
          std::lock_guard<fetch::mutex::Mutex> lock(mutex_);
          if(!heaviestHashes_.empty())
          {
            heaviest = heaviestHashes_.back();
            heaviestHashes_.pop_back();
            lock.unlock();

            std::cout << "Found new heaviest: " << ToHex(heaviest) << std::endl;

            if(executionManager_)
            {
              std::cout << "Manipulate E manager" << std::endl;
            }
          }
          else
          {
            mutex_.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }
        }
      }
    };

    thread_ = std::thread{closure};
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
  chain::MainChain                          &mainChain_;
  std::shared_ptr<ledger::ExecutionManager> executionManager_;
  std::deque<block_hash>                    heaviestHashes_;
  mutex_type                                mutex_{__LINE__, __FILE__};
  bool                                      stop_ = false;
  std::thread                               thread_;

};

}
}

#endif
