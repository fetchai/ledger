#pragma once
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
  using BlockType       = chain::MainChain::BlockType;
  using BlockHash       = chain::MainChain::BlockHash;
  using mutex_type      = fetch::mutex::Mutex;
  using block_body_type = std::shared_ptr<BlockBody>;
  using status_type     = ledger::ExecutionManagerInterface::Status;

  static constexpr char const *LOGGING_NAME = "BlockCoordinator";

  BlockCoordinator(chain::MainChain &chain, ledger::ExecutionManagerInterface &execution_manager)
    : chain_{chain}
    , execution_manager_{execution_manager}
  {}

  ~BlockCoordinator()
  {
    Stop();
  }

  /**
   * Called whenever a new block has been generated from the miner
   *
   * @param block Reference to the new block
   */
  void AddBlock(BlockType &block)
  {
    // add the block to the chain data structure
    chain_.AddBlock(block);

    // TODO(private issue 242): This logic is somewhat flawed, this means that the execution manager
    // does not fire all of the time.
    auto heaviestHash = chain_.HeaviestBlock().hash();

    if (block.hash() == heaviestHash)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "New block: ", ToBase64(block.hash()),
                     " from: ", ToBase64(block.prev()));

      {
        std::lock_guard<mutex_type> lock(mutex_);
        pending_blocks_.push_front(std::make_shared<BlockBody>(block.body()));
      }
    }
    else
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Unscheduled block: ", ToBase64(block.hash()),
                     " Heaviest: ", ToBase64(heaviestHash));
    }
  }

  void Start()
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
        executing_block = execution_manager_.IsActive();

        // debug
        if (!executing_block && block)
        {
          FETCH_LOG_INFO(LOGGING_NAME, "Block Completed: ", ToBase64(block->hash));
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

            if (!pending_blocks_.empty())
            {
              block = pending_blocks_.back();
              pending_blocks_.pop_back();
              schedule_block = true;
            }
          }
        }

        if (schedule_block && block)
        {
          FETCH_LOG_INFO(LOGGING_NAME, "Attempting exec on block: ", ToBase64(block->hash));

          // execute the block
          status_type const status = execution_manager_.Execute(*block);

          if (status == status_type::COMPLETE)
          {
            FETCH_LOG_INFO(LOGGING_NAME, "Block Completed: ", ToBase64(block->hash));
          }
          else if (status == status_type::SCHEDULED)
          {
            FETCH_LOG_INFO(LOGGING_NAME, "Block Scheduled: ", ToBase64(block->hash));
          }
          else if (status == status_type::NO_PARENT_BLOCK)
          {
            BlockType full_block;

            if (chain_.Get(block->previous_hash, full_block))
            {

              // add the current block to the stack
              pending_stack.push_back(block);

              // add the current block to the stack
              block = std::make_shared<BlockBody>(full_block.body());
              pending_stack.push_back(block);

              FETCH_LOG_DEBUG(LOGGING_NAME, "Retrieved parent block: ", ToBase64(block->hash));
            }
            else
            {
              FETCH_LOG_WARN(LOGGING_NAME,
                             "Unable to retreive parent block: ", ToBase64(block->previous_hash));
            }

            // reset the block
            block.reset();
            continue;
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

            FETCH_LOG_WARN(LOGGING_NAME, "Unable to execute block: ", ToBase64(block->hash),
                           " Reason: ", reason);
            block.reset();  // mostly for debug
          }

          executing_block = true;
          schedule_block  = false;
        }

        // wait for the block to process
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
  chain::MainChain &                 chain_;
  ledger::ExecutionManagerInterface &execution_manager_;
  std::deque<block_body_type>        pending_blocks_;
  mutex_type                         mutex_{__LINE__, __FILE__};
  bool                               stop_ = false;
  std::thread                        thread_;
};

}  // namespace chain
}  // namespace fetch
