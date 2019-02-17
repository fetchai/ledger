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
#include "ledger/block_packer_interface.hpp"
#include "ledger/block_sink_interface.hpp"
#include "ledger/chain/consensus/dummy_miner.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/execution_manager_interface.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <chrono>

using std::this_thread::sleep_for;
using std::chrono::microseconds;
using fetch::byte_array::ToBase64;

using ScheduleStatus = fetch::ledger::ExecutionManagerInterface::ScheduleStatus;
using ExecutionState = fetch::ledger::ExecutionManagerInterface::State;

static const std::chrono::milliseconds STALL_INTERVAL{250};
static const std::size_t               STALL_THRESHOLD{20};
static const std::size_t               DIGEST_LENGTH_BYTES{32};
static const std::size_t               IDENTITY_LENGTH_BYTES{64};

namespace fetch {
namespace ledger {

/**
 * Construct the Block Coordinator
 *
 * @param chain The reference to the main change
 * @param execution_manager  The reference to the execution manager
 */
BlockCoordinator::BlockCoordinator(MainChain &chain, ExecutionManagerInterface &execution_manager,
                                   StorageUnitInterface &storage_unit, BlockPackerInterface &packer,
                                   BlockSinkInterface &block_sink, Identity identity,
                                   std::size_t num_lanes, std::size_t num_slices,
                                   std::size_t block_difficulty)
  : chain_{chain}
  , execution_manager_{execution_manager}
  , storage_unit_{storage_unit}
  , block_packer_{packer}
  , block_sink_{block_sink}
  , miner_{std::make_shared<consensus::DummyMiner>()}
  , identity_{std::move(identity)}
  , state_machine_{std::make_shared<StateMachine>("BlockCoordinator", State::RESET)}
  , block_difficulty_{block_difficulty}
  , num_lanes_{num_lanes}
  , num_slices_{num_slices}
{
  // configure the state machine
  // clang-format off
  state_machine_->RegisterHandler(State::SYNCHRONIZING,                this, &BlockCoordinator::OnSynchronizing);
  state_machine_->RegisterHandler(State::SYNCHRONIZED,                 this, &BlockCoordinator::OnSynchronized);
  state_machine_->RegisterHandler(State::PRE_EXEC_BLOCK_VALIDATION,    this, &BlockCoordinator::OnPreExecBlockValidation);
  state_machine_->RegisterHandler(State::SCHEDULE_BLOCK_EXECUTION,     this, &BlockCoordinator::OnScheduleBlockExecution);
  state_machine_->RegisterHandler(State::WAIT_FOR_EXECUTION,           this, &BlockCoordinator::OnWaitForExecution);
  state_machine_->RegisterHandler(State::POST_EXEC_BLOCK_VALIDATION,   this, &BlockCoordinator::OnPostExecBlockValidation);
  state_machine_->RegisterHandler(State::PACK_NEW_BLOCK,               this, &BlockCoordinator::OnPackNewBlock);
  state_machine_->RegisterHandler(State::EXECUTE_NEW_BLOCK,            this, &BlockCoordinator::OnExecuteNewBlock);
  state_machine_->RegisterHandler(State::WAIT_FOR_NEW_BLOCK_EXECUTION, this, &BlockCoordinator::OnWaitForNewBlockExecution);
  state_machine_->RegisterHandler(State::PROOF_SEARCH,                 this, &BlockCoordinator::OnProofSearch);
  state_machine_->RegisterHandler(State::TRANSMIT_BLOCK,               this, &BlockCoordinator::OnTransmitBlock);
  state_machine_->RegisterHandler(State::RESET,                        this, &BlockCoordinator::OnReset);
  // clang-format on

  // for debug purposes
  state_machine_->OnStateChange([](State current, State previous) {
    FETCH_LOG_INFO(LOGGING_NAME, "Changed state: ", ToString(previous), " -> ", ToString(current));
  });
}

/**
 * Destruct the Block Coordinator
 */
BlockCoordinator::~BlockCoordinator()
{
  state_machine_->Reset();
  state_machine_.reset();
}

/**
 * Force the block interval to expire causing the state machine to be able to generate a block if
 * needed
 */
void BlockCoordinator::TriggerBlockGeneration()
{
  if (mining_)
  {
    next_block_time_ = Clock::now();
  }
}

BlockCoordinator::State BlockCoordinator::OnSynchronizing()
{
  State next_state{State::SYNCHRONIZING};

  // ensure that we have a current block that we are executing
  if (!current_block_)
  {
    current_block_ = std::make_shared<Block>(chain_.HeaviestBlock());
  }

  // cache some useful variables
  auto const current_hash         = current_block_->body.hash;
  auto const previous_hash        = current_block_->body.previous_hash;
  auto const desired_state        = current_block_->body.merkle_hash;
  auto const current_state        = storage_unit_.CurrentHash();
  auto const last_processed_block = execution_manager_.LastProcessedBlock();

  FETCH_LOG_INFO(LOGGING_NAME, "Sync: Current......: ", ToBase64(current_hash));
  FETCH_LOG_INFO(LOGGING_NAME, "Sync: Previous.....: ", ToBase64(previous_hash));
  FETCH_LOG_INFO(LOGGING_NAME, "Sync: Desired State: ", ToBase64(desired_state));
  FETCH_LOG_INFO(LOGGING_NAME, "Sync: Current State: ", ToBase64(current_state));
  FETCH_LOG_INFO(LOGGING_NAME, "Sync: last Block...: ", ToBase64(last_processed_block));

  // initial condition, the last processed block is empty
  if (GENESIS_DIGEST == last_processed_block)
  {
    // start up - we need to work out which of the blocks has been executed previously

    if (GENESIS_DIGEST == previous_hash)
    {
      // once we have got back to genesis then we need to start executing from the beginning
      next_state = State::PRE_EXEC_BLOCK_VALIDATION;
    }
    else
    {
      // look up the previous block
      auto previous_block = std::make_shared<Block>();
      if (!chain_.Get(previous_hash, *previous_block))
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Unable to lookup previous block: ", ToBase64(current_hash));
        return State::RESET;
      }

      // update the current block
      current_block_ = previous_block;
    }
  }
  else if (current_hash == last_processed_block)
  {
    // the block coordinator has now successfully synced with the chain of blocks.
    next_state = State::SYNCHRONIZED;
  }
  else
  {
    // normal case - we have processed at least one block

    // find the path
    MainChain::Blocks blocks;
    bool const        lookup_success =
        chain_.GetPathToCommonAncestor(blocks, current_hash, last_processed_block);

    if (!lookup_success)
    {
      FETCH_LOG_INFO(LOGGING_NAME,
                     "Unable to lookup common ancestor for block:", ToBase64(current_hash));
      return State::RESET;
    }

    assert(blocks.size() >= 2);
    assert(!blocks.empty());

    for (auto const &block : blocks)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Sync: Common Path..: ", ToBase64(block.body.hash));
    }

    auto         block_path_it = blocks.crbegin();
    Block const &common_parent = *block_path_it++;
    Block const &next_block    = *block_path_it++;

    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Common Parent: ", ToBase64(common_parent.body.hash));
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Next Block...: ", ToBase64(next_block.body.hash));

    // we expect that the common parent in this case will always have been processed, but this
    // should be checked
    if (!storage_unit_.HashExists(common_parent.body.merkle_hash))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Ancestor block not executed:", ToBase64(current_hash));
      return State::RESET;
    }

    // revert the storage back to the known state
    if (!storage_unit_.RevertToHash(common_parent.body.merkle_hash))
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Unable to restore state for block", ToBase64(current_hash));
      return State::RESET;
    }

    // update the current block and begin scheduling
    current_block_ = std::make_shared<Block>(next_block);
    next_state     = State::PRE_EXEC_BLOCK_VALIDATION;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnSynchronized(State current, State previous)
{
  FETCH_UNUSED(current);
  State next_state{State::SYNCHRONIZED};

  // if we have detected a change in the chain then we need to re-evaluate the chain
  if (chain_.HeaviestBlock().body.hash != current_block_->body.hash)
  {
    next_state = State::RESET;
  }
  else if (mining_ && (Clock::now() >= next_block_time_))
  {
    // create a new block
    auto next_block                = std::make_shared<Block>();
    next_block->body.previous_hash = current_block_->body.hash;
    next_block->body.block_number  = current_block_->body.block_number + 1;
    next_block->body.miner         = identity_;

    // ensure the difficulty is correctly set
    next_block->proof.SetTarget(block_difficulty_);

    // replace the current block
    current_block_ = std::move(next_block);

    // trigger packing state
    next_state = State::PACK_NEW_BLOCK;
  }
  else if (State::SYNCHRONIZING == previous)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Chain Sync complete on ", ToBase64(current_block_->body.hash),
                   " (block: ", current_block_->body.block_number,
                   " prev: ", ToBase64(current_block_->body.previous_hash), ")");
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnPreExecBlockValidation()
{
  bool const is_genesis = current_block_->body.previous_hash == GENESIS_DIGEST;

  // Check: Ensure that we have a previous block
  Block previous{};
  if (!is_genesis)
  {
    if (!chain_.Get(current_block_->body.previous_hash, previous))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Block validation failed: No previous block in chain (",
                     ToBase64(current_block_->body.hash), ")");
      chain_.InvalidateBlock(current_block_->body.hash);
      return State::RESET;
    }

    // Check: Ensure the block number is continuous
    uint64_t const expected_block_number = previous.body.block_number + 1u;
    if (expected_block_number != current_block_->body.block_number)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Block validation failed: Block number mismatch (",
                     ToBase64(current_block_->body.hash), ")");
      chain_.InvalidateBlock(current_block_->body.hash);
      return State::RESET;
    }

    // Check: Ensure the identity is the correct size
    if (IDENTITY_LENGTH_BYTES != current_block_->body.miner.size())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Block validation failed: Miner identity size mismatch (",
                     ToBase64(current_block_->body.hash), ")");
      chain_.InvalidateBlock(current_block_->body.hash);
      return State::RESET;
    }

    // Check: Ensure the number of lanes is correct
    if (num_lanes_ != (1u << current_block_->body.log2_num_lanes))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Block validation failed: Lane count mismatch (",
                     ToBase64(current_block_->body.hash), ")");
      chain_.InvalidateBlock(current_block_->body.hash);
      return State::RESET;
    }

    // Check: Ensure the number of slices is correct
    if (num_slices_ != current_block_->body.slices.size())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Block validation failed: Slice count mismatch (",
                     ToBase64(current_block_->body.hash), ")");
      chain_.InvalidateBlock(current_block_->body.hash);
      return State::RESET;
    }
  }

  // Check: Ensure the digests are the correct size
  if (DIGEST_LENGTH_BYTES != current_block_->body.previous_hash.size())
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Block validation failed: Previous block hash size mismatch (",
                   ToBase64(current_block_->body.hash), ")");
    chain_.InvalidateBlock(current_block_->body.hash);
    return State::RESET;
  }

  // All the checks pass
  return State::SCHEDULE_BLOCK_EXECUTION;
}

BlockCoordinator::State BlockCoordinator::OnScheduleBlockExecution()
{
  State next_state{State::RESET};

  // schedule the current block for execution
  if (ScheduleCurrentBlock())
  {
    next_state = State::WAIT_FOR_EXECUTION;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnWaitForExecution()
{
  State next_state{State::WAIT_FOR_EXECUTION};

  auto const status = QueryExecutorStatus();
  switch (status)
  {
  case ExecutionStatus::IDLE:
    next_state = State::POST_EXEC_BLOCK_VALIDATION;
    break;

  case ExecutionStatus::RUNNING:
    break;

  case ExecutionStatus::STALLED:
    if (stall_count_ < STALL_THRESHOLD)
    {
      ++stall_count_;

      sleep_for(STALL_INTERVAL);
      next_state = State::EXECUTE_NEW_BLOCK;
    }
    else
    {
      next_state = State::RESET;
    }

    break;

  case ExecutionStatus::ERROR:
    next_state = State::RESET;
    break;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnPostExecBlockValidation()
{
  State next_state{State::RESET};

  // Check: Ensure the merkle hash is correct for this block
  auto const state_hash = storage_unit_.CurrentHash();

  bool invalid_block{false};
  if (GENESIS_DIGEST != current_block_->body.previous_hash)
  {
    if (state_hash != current_block_->body.merkle_hash)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Block validation failed: Merkle hash mismatch (block: ",
                     ToBase64(current_block_->body.hash),
                     " expected: ", ToBase64(current_block_->body.merkle_hash),
                     " actual: ", state_hash, ")");

      // signal the block is invalid
      invalid_block = true;
    }
  }

  // After the checks have been completed, if the validation has failed, the system needs to recover
  if (invalid_block)
  {
    bool  revert_successful{false};
    Block previous_block;

    // we need to restore back to the previous block
    if (chain_.Get(current_block_->body.previous_hash, previous_block))
    {
      // signal the storage engine to make these changes
      if (storage_unit_.RevertToHash(previous_block.body.merkle_hash))
      {
        execution_manager_.SetLastProcessedBlock(previous_block.body.hash);
        revert_successful = true;
      }
    }

    // if the revert has gone wrong, we need to initiate a complete resync
    if (!revert_successful)
    {
      storage_unit_.RevertToHash(GENESIS_MERKLE_ROOT);
      execution_manager_.SetLastProcessedBlock(GENESIS_DIGEST);
    }

    // finally mark the block as invalid and purge it from the chain
    chain_.InvalidateBlock(current_block_->body.hash);
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnPackNewBlock()
{
  State next_state{State::RESET};

  try
  {
    // call the block packer
    block_packer_.GenerateBlock(*current_block_, num_lanes_, num_slices_, chain_);

#if 1
    FETCH_LOG_INFO(LOGGING_NAME, "New Block: Hash.........: ", ToBase64(current_block_->body.hash));
    FETCH_LOG_INFO(LOGGING_NAME,
                   "New Block: Previous.....: ", ToBase64(current_block_->body.previous_hash));
    FETCH_LOG_INFO(LOGGING_NAME,
                   "New Block: Merkle.......: ", ToBase64(current_block_->body.merkle_hash));
    FETCH_LOG_INFO(LOGGING_NAME, "New Block: Block Number.: ", current_block_->body.block_number);
    FETCH_LOG_INFO(LOGGING_NAME,
                   "New Block: Miner........: ", ToBase64(current_block_->body.miner));
    FETCH_LOG_INFO(LOGGING_NAME, "New Block: Log2 Lanes...: ", current_block_->body.log2_num_lanes);

    std::size_t slice_index{1};
    for (auto const &slice : current_block_->body.slices)
    {
      if (!slice.empty())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "New Block: Slice........: ", slice_index);

        for (auto const &tx : slice)
        {
          FETCH_LOG_INFO(LOGGING_NAME, "New Block: TX...........: ", ToBase64(tx.transaction_hash),
                         " (", tx.contract_name, ")");
        }
      }

      ++slice_index;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "New Block: Slice........: ", ToBase64(current_block_->body.hash));
#endif

    // update our desired next block time
    UpdateNextBlockTime();

    // trigger the execution of the block
    next_state = State::EXECUTE_NEW_BLOCK;
  }
  catch (std::exception &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Error generated performing block packing: ", ex.what());
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnExecuteNewBlock()
{
  State next_state{State::RESET};

  // schedule the current block for execution
  if (ScheduleCurrentBlock())
  {
    next_state = State::WAIT_FOR_NEW_BLOCK_EXECUTION;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnWaitForNewBlockExecution()
{
  State next_state{State::WAIT_FOR_NEW_BLOCK_EXECUTION};

  auto const status = QueryExecutorStatus();
  switch (status)
  {
  case ExecutionStatus::IDLE:
  {
    // update the current block with the desired hash
    current_block_->body.merkle_hash = storage_unit_.CurrentHash();

    FETCH_LOG_INFO(LOGGING_NAME, "Merkle Hash: ", ToBase64(current_block_->body.merkle_hash));

    next_state = State::PROOF_SEARCH;
    break;
  }

  case ExecutionStatus::RUNNING:
    break;

  case ExecutionStatus::STALLED:

    // TODO(private issue 540): Possibly endless stalling in the error case
    if (stall_count_ < STALL_THRESHOLD)
    {
      ++stall_count_;

      sleep_for(STALL_INTERVAL);
      next_state = State::EXECUTE_NEW_BLOCK;
    }
    else
    {
      next_state = State::RESET;
    }

    break;

  case ExecutionStatus::ERROR:
    next_state = State::RESET;
    break;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnProofSearch()
{
  State next_state{State::PROOF_SEARCH};

  if (miner_->Mine(*current_block_, 100))
  {
    // update the digest
    current_block_->UpdateDigest();

    FETCH_LOG_INFO(LOGGING_NAME, "New Block Hash: ", ToBase64(current_block_->body.hash));

    // this step is needed because the execution manager is actually unaware of the actual last
    // block that is executed because the merkle hash was not known at this point.
    execution_manager_.SetLastProcessedBlock(current_block_->body.hash);

    // the block is now fully formed it can be sent across the network
    next_state = State::TRANSMIT_BLOCK;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnTransmitBlock()
{
  State next_state{State::RESET};

  try
  {
    // ensure that the main chain is aware of the block
    chain_.AddBlock(*current_block_);

    // dispatch the block that has been generated
    block_sink_.OnBlock(*current_block_);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Error transmitting verified block: ", ex.what());
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnReset()
{
  // mostly for debug
  current_block_.reset();
  stall_count_ = 0;

  // we should update the next block time
  UpdateNextBlockTime();

  // TODO(EJF): error handling

  return State::SYNCHRONIZING;
}

bool BlockCoordinator::ScheduleCurrentBlock()
{
  bool success{false};

  // sanity check - ensure there is a block to execute
  if (current_block_)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Attempting exec on block: ", ToBase64(current_block_->body.hash));

    // instruct the execution manager to execute the current block
    auto const execution_status = execution_manager_.Execute(current_block_->body);

    if (execution_status == ScheduleStatus::SCHEDULED)
    {
      // signal success
      success = true;
    }
    else
    {
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Execution engine stalled. State: ", ledger::ToString(execution_status));
    }
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to execute empty block");
  }

  return success;
}

BlockCoordinator::ExecutionStatus BlockCoordinator::QueryExecutorStatus()
{
  ExecutionStatus status{ExecutionStatus::ERROR};

  // based on the state of the execution manager determine
  auto const execution_state = execution_manager_.GetState();

  // map the raw executor status into our simplified version
  switch (execution_state)
  {
  case ExecutionState::IDLE:
    status = ExecutionStatus::IDLE;
    break;

  case ExecutionState::ACTIVE:
    status = ExecutionStatus::RUNNING;
    break;

  case ExecutionState::TRANSACTIONS_UNAVAILABLE:
    status = ExecutionStatus::STALLED;
    break;

  case ExecutionState::EXECUTION_ABORTED:
  case ExecutionState::EXECUTION_FAILED:
    status = ExecutionStatus::ERROR;
    break;
  }

  if (ExecutionStatus::ERROR == status)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Execution in error state: ", ledger::ToString(execution_state));
  }

  return status;
}

void BlockCoordinator::UpdateNextBlockTime()
{
  next_block_time_ = Clock::now() + block_period_;
}

char const *BlockCoordinator::ToString(State state)
{
  char const *text = "Unknown";

  switch (state)
  {
  case State::SYNCHRONIZING:
    text = "Synchronizing";
    break;
  case State::SYNCHRONIZED:
    text = "Synchronized";
    break;
  case State::PRE_EXEC_BLOCK_VALIDATION:
    text = "Pre Block Execution Validation";
    break;
  case State::SCHEDULE_BLOCK_EXECUTION:
    text = "Schedule Block Execution";
    break;
  case State::WAIT_FOR_EXECUTION:
    text = "Waiting for Block Execution";
    break;
  case State::POST_EXEC_BLOCK_VALIDATION:
    text = "Post Block Execution Validation";
    break;
  case State::PACK_NEW_BLOCK:
    text = "Pack New Block";
    break;
  case State::EXECUTE_NEW_BLOCK:
    text = "Execution New Block";
    break;
  case State::WAIT_FOR_NEW_BLOCK_EXECUTION:
    text = "Waiting for New Block Execution";
    break;
  case State::PROOF_SEARCH:
    text = "Searching for Proof";
    break;
  case State::TRANSMIT_BLOCK:
    text = "Transmitting Block";
    break;
  case State::RESET:
    text = "Reset";
    break;
  }

  return text;
}

char const *BlockCoordinator::ToString(ExecutionStatus state)
{
  char const *text = "Unknown";

  switch (state)
  {
  case ExecutionStatus::IDLE:
    text = "Idle";
    break;
  case ExecutionStatus::RUNNING:
    text = "Running";
    break;
  case ExecutionStatus::STALLED:
    text = "Stalled";
    break;
  case ExecutionStatus::ERROR:
    text = "Error";
    break;
  }

  return text;
}

}  // namespace ledger
}  // namespace fetch