#pragma once

#include <miner/miner_interface.hpp>
#include <thread>

#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/execution_manager.hpp"

namespace fetch {
namespace chain {

class BlockCoordinator
{
public:
  using BlockType      = chain::MainChain::BlockType;
  using BlockHash      = chain::MainChain::BlockHash;
  using mutex_type      = fetch::mutex::Mutex;
  using block_body_type = std::shared_ptr<BlockBody>;
  using status_type     = ledger::ExecutionManagerInterface::Status;

  BlockCoordinator(chain::MainChain &mainChain, ledger::ExecutionManagerInterface &executionManager)
    : mainChain_{mainChain}, executionManager_{executionManager}
  {}

  ~BlockCoordinator() { Stop(); }

  void AddBlock(BlockType &block)
  {
    mainChain_.AddBlock(block);

    auto heaviestHash = mainChain_.HeaviestBlock().hash();

    if (block.hash() == heaviestHash)
    {
      fetch::logger.Info("New block: ", ToBase64(block.hash()), " from: ", ToBase64(block.prev()));

      {
        std::lock_guard<mutex_type> lock(mutex_);
        blocksToProcess_.push_front(std::make_shared<BlockBody>(block.body()));
      }
    }
  }

  void start()
  {
    stop_ = false;

    auto closure = [this] {
      block_body_type block;
      bool            executing_block = false;
      bool            schedule_block  = false;

      std::vector<block_body_type> pending_stack;

      while (!stop_)
      {
        // update the status
        executing_block = executionManager_.IsActive();

        // debug
        if (!executing_block && block)
        {
          fetch::logger.Warn("Block Completed: ", ToBase64(block->hash));
          block.reset();
        }

        if (!executing_block)
        {

          // get the next block from the pending queue
          if (!pending_stack.empty())
          {
            block = pending_stack.back();
            pending_stack.pop_back();

            schedule_block = true;
          }
          else
          {
            // get the next block from the queue (if there is one)
            std::lock_guard<mutex_type> lock(mutex_);
            if (!blocksToProcess_.empty())
            {
              block = blocksToProcess_.back();
              blocksToProcess_.pop_back();
              schedule_block = true;
            }
          }
        }

        if (schedule_block && block)
        {
          fetch::logger.Warn("Attempting exec on block: ", ToBase64(block->hash));

          // execute the block
          status_type const status = executionManager_.Execute(*block);

          if (status == status_type::COMPLETE)
          {
            fetch::logger.Warn("Block Completed: ", ToBase64(block->hash));
          }
          else if (status == status_type::SCHEDULED)
          {
            fetch::logger.Warn("Block Scheduled: ", ToBase64(block->hash));
          }
          else if (status == status_type::NO_PARENT_BLOCK)
          {
            BlockType full_block;

            if (mainChain_.Get(block->previous_hash, full_block))
            {

              // add the current block to the stack
              pending_stack.push_back(block);

              // add the current block to the stack
              block = std::make_shared<BlockBody>(full_block.body());
              pending_stack.push_back(block);

              fetch::logger.Warn("Retrieved parent block: ", ToBase64(block->hash));
            }
            else
            {
              fetch::logger.Warn("Unable to retreive parent block: ",
                                 ToBase64(block->previous_hash));
            }

            // reset the block
            block.reset();
            continue;  // erm...
          }
          else
          {

            char const *reason = "unknown";
            switch (status)
            {
            case status_type::COMPLETE:
              reason = "COMPLETE";
              break;
            case status_type::SCHEDULED:
              reason = "SCHEDULED";
              break;
            case status_type::NOT_STARTED:
              reason = "NOT_STARTED";
              break;
            case status_type::ALREADY_RUNNING:
              reason = "ALREADY_RUNNING";
              break;
            case status_type::NO_PARENT_BLOCK:
              reason = "NO_PARENT_BLOCK";
              break;
            case status_type::UNABLE_TO_PLAN:
              reason = "UNABLE_TO_PLAN";
              break;
            }

            fetch::logger.Warn("Unable to execute block: ", ToBase64(block->hash),
                               " Reason: ", reason);
            block.reset();  // mostly for debug
          }

          executing_block = true;
          schedule_block  = false;
        }

        // wait for the block to process
        // std::this_thread::sleep_for(std::chrono::milliseconds(750));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    };

    thread_ = std::thread{closure};
  }

  void Stop()
  {
    if (thread_.joinable())
    {
      stop_ = true;
      thread_.join();
    }
  }

private:
  chain::MainChain &                 mainChain_;
  ledger::ExecutionManagerInterface &executionManager_;
  std::deque<block_body_type>        blocksToProcess_;
  mutex_type                         mutex_{__LINE__, __FILE__};
  bool                               stop_ = false;
  std::thread                        thread_;
};

}  // namespace chain
}  // namespace fetch
