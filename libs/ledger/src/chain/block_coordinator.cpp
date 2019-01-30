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

#include "ledger/chain/block_coordinator.hpp"
#include "core/threading.hpp"

#include <chrono>

using std::this_thread::sleep_for;
using std::chrono::microseconds;

using ScheduleStatus = fetch::ledger::ExecutionManagerInterface::ScheduleStatus;
using ExecutionState = fetch::ledger::ExecutionManagerInterface::State;

static const std::chrono::milliseconds STALL_INTERVAL{250};

namespace fetch {
namespace chain {

/**
 * Construct the Block Coordinator
 *
 * @param chain The reference to the main change
 * @param execution_manager  The reference to the execution manager
 */
BlockCoordinator::BlockCoordinator(chain::MainChain &                 chain,
                                   ledger::ExecutionManagerInterface &execution_manager)
  : chain_{chain}
  , execution_manager_{execution_manager}
{}

/**
 * Destruct the Block Coordinator
 */
BlockCoordinator::~BlockCoordinator()
{
  Stop();
}

/**
 * Called whenever a new block has been generated from the network
 *
 * @param block Reference to the new block
 */
void BlockCoordinator::AddBlock(BlockType &block, bool from_miner)
{
  if (from_miner && callback_)
  {
    callback_(block);
  }
  // add the block to the chain data structure
  chain_.AddBlock(block);

  // TODO(private issue 242): This logic is somewhat flawed, this means that the execution manager
  //                          does not fire all of the time.
  auto const heaviestHash = chain_.HeaviestBlock().hash();

  if (block.hash() == heaviestHash)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "New block: ", ToBase64(block.hash()),
                   " from: ", ToBase64(block.prev()));

    // add the new block into the pending queue
    {
      FETCH_LOCK(pending_blocks_mutex_);
      pending_blocks_.push_front(std::make_shared<BlockBody>(block.body()));
    }
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Unscheduled block: ", ToBase64(block.hash()),
                   " Heaviest: ", ToBase64(heaviestHash));
  }
}

/**
 * Start the Block Coordinator
 */
void BlockCoordinator::Start()
{
  // ensure stop flag is cleared
  stop_ = false;

  // create the new thread to run the monitor
  thread_ = std::thread{&BlockCoordinator::Monitor, this};
}

/**
 * Stp[ the Block Coordinator
 */
void BlockCoordinator::Stop()
{
  if (thread_.joinable())
  {
    stop_ = true;
    thread_.join();
  }
}

/**
 * Monitor the state of the execution manager
 *
 * The role of the monitor to periodically access the status of the execution engine. Depending on
 * its state it will attempt to schedule the next block to be executed. In the case that no new
 * blocks are present it will idle.
 *
 * This monitor is implemented as a state machine which is illustrated in the following diagram:
 *
 * ┌───────────────────────────────┐
 * │                               │
 * │                               ▼
 * │                         ┌───────────┐
 * │                         │   Query   │
 * │                         │ Execution │
 * │                         └───────────┘
 * │                               │
 * │                               │
 * │       ┌───────────────┬───────┴───────┬───────────────┐
 * │       │               │               │               │
 * │       │               │               │               │
 * │       ▼               ▼               ▼               ▼
 * │
 * │   Executing         Idle           Stalled          Error
 * │
 * │       │               │               │               │
 * │       │               │               │               │
 * │       │               ▼               │               │
 * │       │          ┌─────────┐  Block   │               ▼
 * │       │          │Get Next │  Exists  │            ┌ ─ ─ ┐
 * │       │          │  Block  │──────────┤              ???
 * │       │          └─────────┘          │            └ ─ ─ ┘
 * │       │               │               │
 * │       │               │               ▼
 * │       │               │          ┌─────────┐
 * │       │      No Block │          │ Execute │
 * │       │               │          │  Block  │
 * │       │               │          └─────────┘
 * │       │               ▼               │
 * │       │         ┌───────────┐         │
 * │       └────────▶│   Wait    │◀────────┘
 * │                 └───────────┘
 * │                       │
 * │                       │
 * └───────────────────────┘
 *
 * It should be noted that the failed state is not currently handled and will be handles as part
 * of a more comprehensive update to block scheduling as described in #540, #537, #536 and #534.
 */
void BlockCoordinator::Monitor()
{
  SetThreadName("BlockCoord");

  // main state machine loop
  for (;;)
  {
    if (stop_)
    {
      break;
    }

    // execute the correct state handler
    switch (state_)
    {
    case State::QUERY_EXECUTION_STATUS:
      OnQueryExecutionStatus();
      break;
    case State::GET_NEXT_BLOCK:
      OnGetNextBlock();
      break;
    case State::EXECUTE_BLOCK:
      OnExecuteBlock();
      break;
    }

    // allow fast exit/shutdown
    if (stop_)
    {
      break;
    }

    sleep_for(std::chrono::milliseconds(10));
  }
}

/**
 * State: Query the current state of the execution engine
 */
void BlockCoordinator::OnQueryExecutionStatus()
{
  State next_state{State::QUERY_EXECUTION_STATUS};

  // based on the state of the execution manager determine
  auto const execution_state = execution_manager_.GetState();

  switch (execution_state)
  {
  case ExecutionState::IDLE:
    next_state = State::GET_NEXT_BLOCK;
    current_block_.reset();  // this is mostly for debug
    break;

  case ExecutionState::ACTIVE:
    // simple wait for this not to be the case
    break;

  case ExecutionState::TRANSACTIONS_UNAVAILABLE:

    // TODO(private issue 540): Possibly endless stalling waiting for transaction
    sleep_for(STALL_INTERVAL);

    // schedule the execution of the block again (hopefully the transactions would have arrived)
    next_state = State::EXECUTE_BLOCK;
    break;

  case ExecutionState::EXECUTION_ABORTED:
  case ExecutionState::EXECUTION_FAILED:
    FETCH_LOG_WARN(LOGGING_NAME, "Execution in error state: ", ledger::ToString(execution_state));

    // TODO(private issue 540): Possibly endless stalling in the error case
    sleep_for(STALL_INTERVAL);
    break;
  }

  // update the state
  state_ = next_state;
}

/**
 * State: Attempt to lookup a new block to execute
 */
void BlockCoordinator::OnGetNextBlock()
{
  State next_state{State::QUERY_EXECUTION_STATUS};

  // get the next block from the pending queue
  if (!pending_stack_.empty())
  {
    // extract the next element from the pending stack
    current_block_ = pending_stack_.back();
    pending_stack_.pop_back();

    // signal intention to execute the next block
    next_state = State::EXECUTE_BLOCK;
  }
  else
  {
    // get the next block from the queue (if there is one)
    FETCH_LOCK(pending_blocks_mutex_);

    if (!pending_blocks_.empty())
    {
      // extract the next element from the pending queue
      current_block_ = pending_blocks_.back();
      pending_blocks_.pop_back();

      // signal intention to execute the next block
      next_state = State::EXECUTE_BLOCK;
    }
  }

  // update the state
  state_ = next_state;
}

/**
 * State: Attempt to execute the next block
 */
void BlockCoordinator::OnExecuteBlock()
{
  State next_state{State::QUERY_EXECUTION_STATUS};

  // sanity check - ensure there is a block to execute
  if (current_block_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Attempting exec on block: ", ToBase64(current_block_->hash));

    // instruct the execution manager to execute the current block
    auto const execution_status = execution_manager_.Execute(*current_block_);

    switch (execution_status)
    {
    case ScheduleStatus::RESTORED:
      FETCH_LOG_INFO(LOGGING_NAME, "Block Restored: ", ToBase64(current_block_->hash));
      break;

    case ScheduleStatus::SCHEDULED:
      FETCH_LOG_INFO(LOGGING_NAME, "Block Scheduled: ", ToBase64(current_block_->hash));
      break;

    case ScheduleStatus::ALREADY_RUNNING:
      // nothing to be done here except wait for the execution to finish
      break;

    case ScheduleStatus::NO_PARENT_BLOCK:
    {
      Block parent_block;
      if (chain_.Get(current_block_->previous_hash, parent_block))
      {
        // add the current block to the stack
        pending_stack_.push_back(current_block_);
        current_block_.reset();

        // make the parent block the next block to execute
        current_block_ = std::make_shared<BlockBody>(parent_block.body());

        FETCH_LOG_DEBUG(LOGGING_NAME, "Retrieved parent block: ", ToBase64(block->hash));
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to retrieve parent block: ",
                       ToBase64(current_block_->previous_hash));

        // TODO(private issue 540): Possibly endless stalling waiting for block
        sleep_for(STALL_INTERVAL);
      }

      // if no parent block then we need to attempt to reschedule the block
      next_state = State::EXECUTE_BLOCK;

      break;
    }

    case ScheduleStatus::NOT_STARTED:
    case ScheduleStatus::UNABLE_TO_PLAN:
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Execution engine stalled. State: ", ToString(execution_status));
      break;
    }
  }

  // move to the next state
  state_ = next_state;
}

}  // namespace chain
}  // namespace fetch