//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "chain/constants.hpp"
#include "chain/transaction.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/feature_flags.hpp"
#include "core/macros.hpp"
#include "core/set_thread_name.hpp"
#include "core/time/to_seconds.hpp"
#include "ledger/block_packer_interface.hpp"
#include "ledger/block_sink_interface.hpp"
#include "ledger/chain/block_coordinator.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/dag/dag_interface.hpp"
#include "ledger/execution_manager_interface.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "ledger/upow/synergetic_execution_manager.hpp"
#include "ledger/upow/synergetic_executor.hpp"
#include "network/generics/milli_timer.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

using fetch::generics::MilliTimer;

namespace fetch {
namespace ledger {
namespace {

using fetch::byte_array::ToBase64;

using ScheduleStatus       = fetch::ledger::ExecutionManagerInterface::ScheduleStatus;
using ExecutionState       = fetch::ledger::ExecutionManagerInterface::State;
using SynergeticExecMgrPtr = std::unique_ptr<SynergeticExecutionManagerInterface>;
using SynergeticMinerPtr   = std::unique_ptr<SynergeticMinerInterface>;
using ProverPtr            = BlockCoordinator::ProverPtr;
using DAGPtr               = std::shared_ptr<ledger::DAGInterface>;

// Constants
const std::chrono::milliseconds TX_SYNC_NOTIFY_INTERVAL{1000};
const std::chrono::milliseconds EXEC_NOTIFY_INTERVAL{5000};
const std::chrono::seconds      STATE_NOTIFY_INTERVAL{20};
const std::chrono::seconds      NOTIFY_INTERVAL{5};
const std::chrono::seconds      WAIT_BEFORE_ASKING_FOR_MISSING_TX_INTERVAL{5};
const std::size_t               MIN_BLOCK_SYNC_SLIPPAGE_FOR_WAITLESS_SYNC_OF_MISSING_TXS{30};
const std::chrono::seconds      WAIT_FOR_TX_TIMEOUT_INTERVAL{120};
const uint32_t                  THRESHOLD_FOR_FAST_SYNCING{100u};

}  // namespace

/**
 * Construct the Block Coordinator
 *
 * @param chain The reference to the main change
 * @param execution_manager  The reference to the execution manager
 */
BlockCoordinator::BlockCoordinator(MainChain &chain, DAGPtr dag,
                                   ExecutionManagerInterface &execution_manager,
                                   StorageUnitInterface &storage_unit, BlockPackerInterface &packer,
                                   BlockSinkInterface &block_sink, ProverPtr prover,
                                   uint32_t log2_num_lanes, std::size_t num_slices,
                                   ConsensusPtr         consensus,
                                   SynergeticExecMgrPtr synergetic_exec_manager)
  : chain_{chain}
  , dag_{std::move(dag)}
  , consensus_{std::move(consensus)}
  , execution_manager_{execution_manager}
  , storage_unit_{storage_unit}
  , block_packer_{packer}
  , block_sink_{block_sink}
  , periodic_print_{STATE_NOTIFY_INTERVAL}
  , last_executed_block_{chain::ZERO_HASH}
  , certificate_{std::move(prover)}
  , mining_address_{certificate_->identity()}
  , state_machine_{std::make_shared<StateMachine>("BlockCoordinator", State::RELOAD_STATE,
                                                  [](State state) { return ToString(state); })}
  , log2_num_lanes_{log2_num_lanes}
  , num_slices_{num_slices}
  , tx_wait_periodic_{TX_SYNC_NOTIFY_INTERVAL}
  , exec_wait_periodic_{EXEC_NOTIFY_INTERVAL}
  , syncing_periodic_{NOTIFY_INTERVAL}
  , synergetic_exec_mgr_{std::move(synergetic_exec_manager)}
  , reload_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_reload_state_total",
        "The total number of times in the reload state")}
  , synchronising_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_synchronising_state_total",
        "The total number of times in the synchronising state")}
  , synchronised_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_synchronised_state_total",
        "The total number of times in the synchronised state")}
  , pre_valid_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_pre_valid_state_total",
        "The total number of times in the pre validation state")}
  , wait_tx_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_wait_tx_state_total",
        "The total number of times in the wait for tx state")}
  , syn_exec_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_syn_exec_state_total",
        "The total number of times in the synergetic execution state")}
  , sch_block_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_sch_block_state_total",
        "The total number of times in the schedule block exec state")}
  , wait_exec_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_wait_exec_state_total",
        "The total number of times in the waiting for exec state")}
  , post_valid_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_post_valid_state_total",
        "The total number of times in the post validation state")}
  , pack_block_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_pack_block_state_total",
        "The total number of times in the pack new block state")}
  , new_syn_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_new_syn_state_total",
        "The total number of times in the new synergetic state")}
  , new_exec_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_new_exec_state_total",
        "The total number of times in the new synergetic exec state")}
  , new_wait_exec_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_new_wait_exec_state_total",
        "The total number of times in the new wait exec state")}
  , transmit_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_transmit_state_total",
        "The total number of times in the transmit state")}
  , reset_state_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_reset_state_total",
        "The total number of times in the reset state")}
  , executed_block_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_executed_block_total", "The total number of executed blocks")}
  , mined_block_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_mined_block_total", "The total number of mined blocks")}
  , executed_tx_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_executed_tx_total", "The total number of executed transactions")}
  , request_tx_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_request_tx_total",
        "The total number of times an explicit request for transactions was made")}
  , unable_to_find_tx_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_block_coordinator_invalidated_tx_total",
        "The total number of times a block was invalidated because transactions were not found")}
  , blocks_minted_{telemetry::Registry::Instance().CreateCounter("blocks_minted_total",
                                                                 "Blocks minted")}
  , consensus_update_failure_total_{telemetry::Registry::Instance().CreateCounter(
        "consensus_update_failure_total", "Failures to update consensus")}
  , tx_sync_times_{telemetry::Registry::Instance().CreateHistogram(
        {0.001, 0.01, 0.1, 1, 10, 100}, "ledger_block_coordinator_tx_sync_times",
        "The histogram of the time it takes to sync transactions")}
  , current_block_num_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "ledger_latest_block_num",
        "The lastest block number that has been executed by the block coordinator")}
  , next_block_num_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "ledger_next_block_num",
        "The number of the next block which is scheduled to be executed by the block coordinator")}
  , block_hash_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "block_hash", "The last seen block hash beginning")}
  , total_time_to_create_block_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "total_time_to_create_block", "Total time required to create a block")}
  , current_block_weight_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "current_block_weight", "Weight of current block")}
  , last_block_interval_s_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "last_block_interval_s", "Measured block interval")}
  , current_block_coord_state_{telemetry::Registry::Instance().CreateGauge<uint64_t>(
        "current_block_coord_state", "Current block coord state")}
{
  // configure the state machine
  // clang-format off
  state_machine_->RegisterHandler(State::RELOAD_STATE,                 this, &BlockCoordinator::OnReloadState);
  state_machine_->RegisterHandler(State::SYNCHRONISING,                this, &BlockCoordinator::OnSynchronising);
  state_machine_->RegisterHandler(State::SYNCHRONISED,                 this, &BlockCoordinator::OnSynchronised);

  // Pipe 1
  state_machine_->RegisterHandler(State::PRE_EXEC_BLOCK_VALIDATION,    this, &BlockCoordinator::OnPreExecBlockValidation);
  state_machine_->RegisterHandler(State::SYNERGETIC_EXECUTION,         this, &BlockCoordinator::OnSynergeticExecution);
  state_machine_->RegisterHandler(State::WAIT_FOR_TRANSACTIONS,        this, &BlockCoordinator::OnWaitForTransactions);
  state_machine_->RegisterHandler(State::SCHEDULE_BLOCK_EXECUTION,     this, &BlockCoordinator::OnScheduleBlockExecution);
  state_machine_->RegisterHandler(State::WAIT_FOR_EXECUTION,           this, &BlockCoordinator::OnWaitForExecution);
  state_machine_->RegisterHandler(State::POST_EXEC_BLOCK_VALIDATION,   this, &BlockCoordinator::OnPostExecBlockValidation);

  // Pipe 2
  state_machine_->RegisterHandler(State::PACK_NEW_BLOCK,               this, &BlockCoordinator::OnPackNewBlock);
  state_machine_->RegisterHandler(State::NEW_SYNERGETIC_EXECUTION,     this, &BlockCoordinator::OnNewSynergeticExecution);
  state_machine_->RegisterHandler(State::EXECUTE_NEW_BLOCK,            this, &BlockCoordinator::OnExecuteNewBlock);
  state_machine_->RegisterHandler(State::WAIT_FOR_NEW_BLOCK_EXECUTION, this, &BlockCoordinator::OnWaitForNewBlockExecution);

  state_machine_->RegisterHandler(State::TRANSMIT_BLOCK,               this, &BlockCoordinator::OnTransmitBlock);
  state_machine_->RegisterHandler(State::RESET,                        this, &BlockCoordinator::OnReset);
  // clang-format on

  assert(consensus_);

  state_machine_->OnStateChange([this](State current, State previous) {
    FETCH_UNUSED(this);
    FETCH_UNUSED(current);
    FETCH_UNUSED(previous);
    if (periodic_print_.Poll())
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Current state: ", ToString(current),
                      " (previous: ", ToString(previous), ")");
    }
  });

  // TODO(private issue 792): this shouldn't be here, but if it is, it locks the whole system on
  // startup. RecoverFromStartup();
}

// Reload state ONCE on first start up of the block coordinator. Attempt to set
// it up as if the shutdown didn't happen
BlockCoordinator::State BlockCoordinator::OnReloadState()
{
  MilliTimer const timer{"OnReload ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  reload_state_count_->increment();

  // By default we need to populate this.
  current_block_ = MainChain::CreateGenesisBlock();

  FETCH_LOG_INFO(LOGGING_NAME, "Loading block coordinator old state...");

  auto block = chain_.GetHeaviestBlock();

  if (block->IsGenesis())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "The main chain's heaviest is genesis. Nothing to load.");
    return State::RESET;
  }

  // Walk back down the chain until we find a state we can revert to
  while (block && !storage_unit_.HashExists(block->merkle_hash, block->block_number))
  {
    block = chain_.GetBlock(block->previous_hash);
  }

  if (!block)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to walk back the chain when recovering!");
  }

  if (block && storage_unit_.HashExists(block->merkle_hash, block->block_number))
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Found a block to revert to! Block: ", block->block_number,
                   " hex: 0x", block->hash.ToHex(), " merkle hash: 0x", block->merkle_hash.ToHex());

    if (!storage_unit_.RevertToHash(block->merkle_hash, block->block_number))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "The revert operation failed!");
      return State::RESET;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Reverted storage unit.");

    // Need to revert the DAG too
    if (dag_ && !dag_->RevertToEpoch(block->block_number))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Reverting the DAG failed!");
      return State::RESET;
    }

    FETCH_LOG_INFO(LOGGING_NAME, "reverted dag.");

    // we need to update the execution manager state and also our locally cached state about the
    // 'last' block that has been executed
    execution_manager_.SetLastProcessedBlock(block->hash);
    last_executed_block_.ApplyVoid([&block](auto &digest) { digest = block->hash; });
    current_block_ = block;

    FETCH_LOG_INFO(LOGGING_NAME, "Success.");
  }
  else
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Didn't find any prior merkle state to revert to.");
  }

  return State::RESET;
}

BlockCoordinator::State BlockCoordinator::OnSynchronising()
{
  MilliTimer const timer{"OnSynchronising ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  synchronising_state_count_->increment();

  // ensure that we have a current block that we are executing
  if (!current_block_)
  {
    current_block_ = chain_.GetHeaviestBlock();
  }

  if (!current_block_ || current_block_->hash.empty())
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Invalid heaviest block, empty block hash");

    state_machine_->Delay(std::chrono::milliseconds{500});
    return State::RESET;
  }

  // update the current block telemetry
  current_block_num_->set(current_block_->block_number);

  // determine if extra debug is wanted or needed
  bool const extra_debug = syncing_periodic_.Poll();

  // cache some useful variables
  auto const     current_hash         = current_block_->hash;
  auto const     previous_hash        = current_block_->previous_hash;
  auto const     desired_state        = current_block_->merkle_hash;
  auto const     last_committed_state = storage_unit_.LastCommitHash();
  auto const     current_state        = storage_unit_.CurrentHash();
  auto const     last_processed_block = execution_manager_.LastProcessedBlock();
  uint64_t const current_dag_epoch    = dag_ ? dag_->CurrentEpoch() : 0;
  bool const     is_genesis           = current_block_->IsGenesis();

#ifdef FETCH_LOG_DEBUG_ENABLED
  if (extra_debug)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Heaviest.....: 0x", chain_.GetHeaviestBlockHash().ToHex());
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Current......: 0x", current_hash.ToHex());
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Previous.....: 0x", previous_hash.ToHex());
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Desired State: 0x", desired_state.ToHex());
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Current State: 0x", current_state.ToHex());
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: LCommit State: 0x", last_committed_state.ToHex());
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Last Block...: 0x", last_processed_block.ToHex());
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Last BlockInt: 0x",
                   last_executed_block_.Apply([](auto const &hash) { return hash; }).ToHex());
    FETCH_LOG_INFO(LOGGING_NAME, "Sync: Last DAGEpoch: 0x", current_dag_epoch);
  }
#endif  // FETCH_LOG_DEBUG_ENABLED

  FETCH_UNUSED(current_dag_epoch);

  // initial condition, the last processed block is empty
  if (chain::ZERO_HASH == last_processed_block)
  {
    // start up - we need to work out which of the blocks has been executed previously

    if (is_genesis)
    {
      // once we have got back to genesis then we need to start executing from the beginning
      return State::PRE_EXEC_BLOCK_VALIDATION;
    }

    // look up the previous block
    auto previous_block = chain_.GetBlock(previous_hash);
    if (!previous_block)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to look up previous block: ", ToBase64(current_hash));
      return State::RESET;
    }

    // update the current block
    current_block_ = previous_block;
  }
  else if (current_hash == last_processed_block)
  {
    // the block coordinator has now successfully synced with the chain of blocks.
    return State::SYNCHRONISED;
  }
  else
  {
    // normal case - we have processed at least one block

    // find the path to ancestor - retain this path if it is long for efficiency reasons.
    bool lookup_success = false;

    if (blocks_to_common_ancestor_.empty())
    {
      lookup_success = chain_.GetPathToCommonAncestor(
          blocks_to_common_ancestor_, current_hash, last_processed_block,
          COMMON_PATH_TO_ANCESTOR_LENGTH_LIMIT, MainChain::BehaviourWhenLimit::RETURN_LEAST_RECENT);
    }
    else
    {
      lookup_success = true;
    }

    if (!lookup_success)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Unable to look up common ancestor for block:", ToBase64(current_hash));
      return State::RESET;
    }

    assert(blocks_to_common_ancestor_.size() >= 2 &&
           "Expected at least two blocks from common ancestor: HEAD and current");

    auto     block_path_it = blocks_to_common_ancestor_.crbegin();
    BlockPtr common_parent = *block_path_it++;
    BlockPtr next_block    = *block_path_it++;

    // update the telemetry
    next_block_num_->set(next_block->block_number);

    if (extra_debug)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Sync: Common Parent: 0x", common_parent->hash.ToHex());
      FETCH_LOG_DEBUG(LOGGING_NAME, "Sync: Next Block...: 0x", next_block->hash.ToHex());

      // calculate a percentage synchronisation
      std::size_t const current_block_num = next_block->block_number;
      std::size_t const total_block_num   = current_block_->block_number;
      double const      completion =
          static_cast<double>(current_block_num * 100) / static_cast<double>(total_block_num);

      FETCH_LOG_INFO(LOGGING_NAME, "Synchronising of chain in progress. ", completion, "% (block ",
                     next_block->block_number, " of ", current_block_->block_number, ")");
    }

    // we expect that the common parent in this case will always have been processed, but this
    // should be checked
    if (!storage_unit_.HashExists(common_parent->merkle_hash, common_parent->block_number))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Ancestor block's merkle hash cannot be retrieved! block: 0x",
                      current_hash.ToHex(), " number: ", common_parent->block_number,
                      " merkle hash: 0x", common_parent->merkle_hash.ToHex(),
                      " Last processed: ", last_processed_block.ToHex());

      // this is a bad situation so the easiest solution is to revert back to genesis
      execution_manager_.SetLastProcessedBlock(chain::ZERO_HASH);
      if (!storage_unit_.RevertToHash(chain::GetGenesisMerkleRoot(), 0))
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Unable to revert back to genesis");
      }

      if (dag_ && !dag_->RevertToEpoch(0))
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Unable to revert DAG back to genesis!");
      }

      // delay the state machine in these error cases, to allow the network to catch up if the issue
      // is network related and if nothing else restrict logs being spammed
      state_machine_->Delay(std::chrono::seconds{5});

      return State::RESET;
    }

    // revert the storage back to the known state
    if (!storage_unit_.RevertToHash(common_parent->merkle_hash, common_parent->block_number))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Unable to restore state for block: ", current_hash.ToHex());

      // delay the state machine in these error cases, to allow the network to catch up if the issue
      // is network related and if nothing else restrict logs being spammed
      state_machine_->Delay(std::chrono::seconds{5});

      return State::RESET;
    }

    if (dag_ && !dag_->RevertToEpoch(common_parent->block_number))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to revert dag to block: ", common_parent->block_number);
      state_machine_->Delay(std::chrono::seconds{5});
      return State::RESET;
    }

    // update the current block and begin scheduling
    current_block_ = next_block;

    blocks_to_common_ancestor_.pop_back();

    if (blocks_to_common_ancestor_.size() < THRESHOLD_FOR_FAST_SYNCING)
    {
      blocks_to_common_ancestor_.clear();
    }

    return State::PRE_EXEC_BLOCK_VALIDATION;
  }

  return State::SYNCHRONISING;
}

BlockCoordinator::State BlockCoordinator::OnSynchronised(State current, State previous)
{
  MilliTimer const timer{"OnSynchronised ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  synchronised_state_count_->increment();
  FETCH_UNUSED(current);

  if (State::SYNCHRONISING == previous)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Chain Sync complete on 0x", current_block_->hash.ToHex(),
                   " (block: ", current_block_->block_number, " prev: 0x",
                   current_block_->previous_hash.ToHex(), ")");
  }

  // Telemetry
  {
    if (current_block_ && !current_block_->IsGenesis())
    {
      BlockPtr previous_block = chain_.GetBlock(current_block_->previous_hash);

      if (previous_block)
      {
        last_block_interval_s_->set(current_block_->timestamp - previous_block->timestamp);
      }
    }
  }

  // ensure the periodic print is not trigger once we have synced
  syncing_periodic_.Reset();

  // if we have detected a change in the chain then we need to re-evaluate the chain
  if (chain_.GetHeaviestBlockHash() != current_block_->hash)
  {
    return State::RESET;
  }

  try
  {
    consensus_->UpdateCurrentBlock(*current_block_);

    // Failure will set this to a nullptr
    next_block_ = consensus_->GenerateNextBlock();
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to update consensus with error: ", ex.what());
    consensus_update_failure_total_->increment();

    return State::RESET;
  }
  catch (...)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unknown error when updating consensus!");
    consensus_update_failure_total_->increment();
    next_block_ = nullptr;

    return State::RESET;
  }

  if (!next_block_)
  {
    state_machine_->Delay(std::chrono::milliseconds{100});
    return State::SYNCHRONISED;
  }

  next_block_->previous_hash  = current_block_->hash;
  next_block_->block_number   = current_block_->block_number + 1;
  next_block_->log2_num_lanes = log2_num_lanes_;

  FETCH_LOG_DEBUG(LOGGING_NAME, "Minting new block! Number: ", next_block_->block_number,
                  " beacon: ", next_block_->block_entropy.EntropyAsU64());

  start_block_packing_ = Clock::now();

  // Attach current DAG state
  if (dag_)
  {
    next_block_->dag_epoch = dag_->CreateEpoch(next_block_->block_number);
  }

  // discard the current block (we are making a new one)
  current_block_.reset();

  // trigger packing state
  return State::NEW_SYNERGETIC_EXECUTION;
}

BlockCoordinator::State BlockCoordinator::OnPreExecBlockValidation()
{
  MilliTimer const timer{"OnPreExecBlockValidation ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  pre_valid_state_count_->increment();

  bool const is_genesis = current_block_->IsGenesis();

  if (!is_genesis)
  {
    BlockPtr                   previous = chain_.GetBlock(current_block_->previous_hash);
    ConsensusInterface::Status result   = ConsensusInterface::Status::NO;

    try
    {
      result = consensus_->ValidBlock(*current_block_);
    }
    catch (...)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unknown error when validating block!");
      consensus_update_failure_total_->increment();
    }

    if (!(result == ConsensusInterface::Status::YES))
    {
      FETCH_LOG_ERROR(LOGGING_NAME,
                      "Block validation failed: Block coordinator failed to verify block (0x",
                      current_block_->hash.ToHex(), ')', ". This should not happen.");

      RemoveBlock(current_block_->hash);
      Reset();
      return State::RESET;
    }

    try
    {
      consensus_->UpdateCurrentBlock(*previous);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to update consensus with error: ", ex.what());
      consensus_update_failure_total_->increment();
    }
    catch (...)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unknown error when updating consensus!");
      consensus_update_failure_total_->increment();
    }

    // Check: Ensure the number of lanes is correct
    if (num_lanes_ != (1u << current_block_->log2_num_lanes))
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Block validation failed: Lane count mismatch. Expected: ", num_lanes_,
                     " Actual: ", (1u << current_block_->log2_num_lanes), " (0x",
                     current_block_->hash.ToHex(), ')');

      RemoveBlock(current_block_->hash);
      return State::RESET;
    }

    // Check: Ensure the number of slices is correct
    if (num_slices_ != current_block_->slices.size())
    {
      FETCH_LOG_WARN(
          LOGGING_NAME, "Block validation failed: Slice count mismatch. Expected: ", num_slices_,
          " Actual: ", current_block_->slices.size(), " (0x", current_block_->hash.ToHex(), ')');

      RemoveBlock(current_block_->hash);
      return State::RESET;
    }
  }

  // Validating DAG hashes
  if ((!is_genesis) && synergetic_exec_mgr_)
  {
    BlockPtr previous_block = chain_.GetBlock(current_block_->previous_hash);

    // All work is identified on the latest DAG segment and prepared in a queue
    auto const result = synergetic_exec_mgr_->PrepareWorkQueue(*current_block_, *previous_block);
    if (SynExecStatus::SUCCESS != result)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Block certifies work that possibly is malicious (0x",
                     current_block_->hash.ToHex(), ")");

      RemoveBlock(current_block_->hash);
      return State::RESET;
    }
  }

  // reset the tx wait period
  tx_wait_periodic_.Reset();

  // All the checks pass
  return State::WAIT_FOR_TRANSACTIONS;
}

BlockCoordinator::State BlockCoordinator::OnSynergeticExecution()
{
  MilliTimer const timer{"OnSynergeticExecution ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  syn_exec_state_count_->count();

  bool const is_genesis = current_block_->IsGenesis();

  // Executing synergetic work
  if ((!is_genesis) && synergetic_exec_mgr_)
  {
    // look up the previous block
    auto const previous_block = chain_.GetBlock(current_block_->previous_hash);
    if (!previous_block)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to look up previous block");
      return State::RESET;
    }

    // prepare the work queue
    auto const status = synergetic_exec_mgr_->PrepareWorkQueue(*current_block_, *previous_block);
    if (SynExecStatus::SUCCESS != status)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Error preparing synergetic work queue: ", ledger::ToString(status));
      return State::RESET;
    }

    if (!synergetic_exec_mgr_->ValidateWorkAndUpdateState(num_lanes_))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Work did not execute (0x", current_block_->hash.ToHex(), ")");
      RemoveBlock(current_block_->hash);

      return State::RESET;
    }
  }

  return State::SCHEDULE_BLOCK_EXECUTION;
}

BlockCoordinator::State BlockCoordinator::OnWaitForTransactions(State current, State previous)
{
  MilliTimer const timer{"OnWaitForTransactions ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  wait_tx_state_count_->increment();

  if (previous == current)
  {
    if (have_asked_for_missing_txs_)
    {
      // FSM is stuck waiting for transactions - has timeout elapsed?
      if (wait_for_tx_timeout_.HasExpired())
      {
        unable_to_find_tx_count_->increment();

        // Assume block was invalid and discard it
        RemoveBlock(current_block_->hash);

        return State::RESET;
      }
    }
    else
    {
      auto const distance_from_heaviest_block{chain_.GetHeaviestBlock()->block_number -
                                              current_block_->block_number};

      auto const is_waitless_syncing_enabled{
          distance_from_heaviest_block > MIN_BLOCK_SYNC_SLIPPAGE_FOR_WAITLESS_SYNC_OF_MISSING_TXS};

      if (is_waitless_syncing_enabled || wait_before_asking_for_missing_tx_.HasExpired())
      {
        request_tx_count_->increment();

        FETCH_LOG_WARN(LOGGING_NAME, "OnWaitForTransactions: Calling IssueCallForMissingTxs for ",
                       pending_txs_->size(), " TXs (for block: ", current_block_->block_number,
                       ", heaviest block: ", chain_.GetHeaviestBlock()->block_number, ")");

        storage_unit_.IssueCallForMissingTxs(*pending_txs_);
        have_asked_for_missing_txs_ = true;
        wait_for_tx_timeout_.Restart(WAIT_FOR_TX_TIMEOUT_INTERVAL);
      }
    }
  }
  else  // this is the first time in this state
  {
    // Only just started waiting for transactions - reset countdown to issuing request to peers
    wait_before_asking_for_missing_tx_.Restart(WAIT_BEFORE_ASKING_FOR_MISSING_TX_INTERVAL);
    have_asked_for_missing_txs_ = false;
    start_waiting_for_tx_       = Clock::now();  // cache the start time
  }

  // TODO(HUT): this might need to check that storage has whatever this dag epoch needs wrt
  // contracts.
  bool dag_is_ready{true};

  if (dag_)
  {
    // This combines waiting until all dag nodes are in the epoch and epoch validation (well formed
    // dag)
    dag_is_ready = dag_->SatisfyEpoch(current_block_->dag_epoch);
  }

  // if the transaction digests have not been cached then do this now
  if (!pending_txs_)
  {
    pending_txs_ = std::make_unique<DigestSet>();

    for (auto const &slice : current_block_->slices)
    {
      for (auto const &tx : slice)
      {
        pending_txs_->insert(tx.digest());
      }
    }
  }

  // evaluate if the transactions have arrived
  auto it = pending_txs_->begin();
  while (it != pending_txs_->end())
  {
    if (storage_unit_.HasTransaction(*it))
    {
      // success - remove this element from the set
      it = pending_txs_->erase(it);
    }
    else
    {
      // otherwise advance on to the next one
      ++it;
    }
  }

  // once all the transactions are present we can then move to scheduling the block. This makes life
  // much easier all around
  if (pending_txs_->empty() && dag_is_ready)
  {
    // record the time this successful syncing took place
    tx_sync_times_->Add(ToSeconds(Clock::now() - start_waiting_for_tx_));

    FETCH_LOG_DEBUG(LOGGING_NAME, "All transactions have been synchronised!");

    // clear the pending transaction set
    pending_txs_.reset();

    return State::SYNERGETIC_EXECUTION;
  }

  // status debug
  if (tx_wait_periodic_.Poll())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Waiting for ", pending_txs_->size(), " transactions to sync");
  }

  if (!dag_is_ready)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Waiting for DAG to sync");
  }

  // signal the next execution of the state machine should be much later in the future
  state_machine_->Delay(std::chrono::milliseconds{200});

  return State::WAIT_FOR_TRANSACTIONS;
}

void BlockCoordinator::RemoveBlock(MainChain::BlockHash const &hash)
{
  chain_.RemoveBlock(hash);
  blocks_to_common_ancestor_.clear();
}

BlockCoordinator::State BlockCoordinator::OnScheduleBlockExecution()
{

  MilliTimer const timer{"OnScheduleBlockExecution ", 1000};
  sch_block_state_count_->increment();

  State next_state{State::RESET};

  // schedule the current block for execution
  if (ScheduleCurrentBlock())
  {
    exec_wait_periodic_.Reset();

    next_state = State::WAIT_FOR_EXECUTION;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnWaitForExecution()
{
  MilliTimer const timer{"OnWaitForExecution ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  wait_exec_state_count_->increment();

  State next_state{State::WAIT_FOR_EXECUTION};

  auto const status = QueryExecutorStatus();

  switch (status)
  {
  case ExecutionStatus::IDLE:
    next_state = State::POST_EXEC_BLOCK_VALIDATION;
    break;

  case ExecutionStatus::RUNNING:

    if (exec_wait_periodic_.Poll())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Waiting for execution to complete for block: 0x",
                     current_block_->hash.ToHex());
    }

    // signal that the next execution should not happen immediately
    state_machine_->Delay(std::chrono::milliseconds{20});
    break;

  case ExecutionStatus::STALLED:
  case ExecutionStatus::ERROR:
    next_state = State::RESET;
    break;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnPostExecBlockValidation()
{
  MilliTimer const timer{"OnPostExecBlockValidation ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  post_valid_state_count_->increment();

  // Check: Ensure the merkle hash is correct for this block
  auto const state_hash = storage_unit_.CurrentHash();

  bool invalid_block{false};
  if (!current_block_->IsGenesis())
  {
    if (state_hash != current_block_->merkle_hash)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Block validation failed: Merkle hash mismatch (block num: ",
                     current_block_->block_number, " block: 0x", current_block_->hash.ToHex(),
                     " expected: 0x", current_block_->merkle_hash.ToHex(), " actual: 0x",
                     state_hash.ToHex(), ")");

      // signal the block is invalid
      invalid_block = true;
    }
    else
    {
      FETCH_LOG_DEBUG(LOGGING_NAME,
                      "Block validation great success: (block num: ", current_block_->block_number,
                      " block: 0x", current_block_->hash.ToHex(), " expected: 0x",
                      current_block_->merkle_hash.ToHex(), " actual: 0x", state_hash.ToHex(), ")");
    }
  }

  // After the checks have been completed, if the validation has failed, the system needs to recover
  if (invalid_block)
  {
    bool revert_successful{false};

    // we need to restore back to the previous block
    BlockPtr previous_block = chain_.GetBlock(current_block_->previous_hash);
    if (previous_block)
    {
      if (dag_)
      {
        revert_successful = dag_->RevertToEpoch(previous_block->block_number);
      }

      // signal the storage engine to make these changes
      if (storage_unit_.RevertToHash(previous_block->merkle_hash, previous_block->block_number) &&
          revert_successful)
      {
        execution_manager_.SetLastProcessedBlock(previous_block->hash);
        revert_successful = true;
      }
    }

    // if the revert has gone wrong, we need to initiate a complete re-sync
    if (!revert_successful)
    {
      if (dag_)
      {
        dag_->RevertToEpoch(0);
      }
      storage_unit_.RevertToHash(chain::GetGenesisMerkleRoot(), 0);
      execution_manager_.SetLastProcessedBlock(chain::ZERO_HASH);
    }

    // finally mark the block as invalid and purge it from the chain
    RemoveBlock(current_block_->hash);
  }
  else
  {
    // Commit this state
    storage_unit_.Commit(current_block_->block_number);

    // Notify the DAG of this epoch
    if (dag_)
    {
      dag_->CommitEpoch(current_block_->dag_epoch);
    }

    // signal the last block that has been executed
    last_executed_block_.ApplyVoid([this](auto &digest) { digest = current_block_->hash; });

    // update the telemetry
    executed_block_count_->increment();
    executed_tx_count_->add(current_block_->GetTransactionCount());

    try
    {
      consensus_->UpdateCurrentBlock(*current_block_);
    }
    catch (std::exception const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Failed to update consensus with valid block, with error: ", ex.what());
      consensus_update_failure_total_->increment();
    }
    catch (...)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unknown error when updating consensus!");
      consensus_update_failure_total_->increment();
    }
  }

  return State::RESET;
}

BlockCoordinator::State BlockCoordinator::OnPackNewBlock()
{
  MilliTimer const timer{"OnPackNewBlock ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  pack_block_state_count_->increment();

  State next_state{State::RESET};

  try
  {
    // call the block packer
    block_packer_.GenerateBlock(*next_block_, num_lanes_, num_slices_, chain_);

    // trigger the execution of the block
    next_state = State::EXECUTE_NEW_BLOCK;
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Error generated performing block packing: ", ex.what());
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnNewSynergeticExecution()
{
  MilliTimer const timer{"OnNewSynergeticExecution ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  new_syn_state_count_->increment();

  if (synergetic_exec_mgr_ && dag_)
  {
    assert(!next_block_->IsGenesis());
    // look up the previous block
    BlockPtr previous_block = chain_.GetBlock(next_block_->previous_hash);

    // prepare the work queue
    auto const status = synergetic_exec_mgr_->PrepareWorkQueue(*next_block_, *previous_block);
    if (SynExecStatus::SUCCESS != status)
    {
      FETCH_LOG_WARN(LOGGING_NAME,
                     "Error preparing synergetic work queue: ", ledger::ToString(status));
      return State::RESET;
    }

    if (!synergetic_exec_mgr_->ValidateWorkAndUpdateState(num_lanes_))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to valid work queue");

      return State::RESET;
    }
  }

  return State::PACK_NEW_BLOCK;
}

BlockCoordinator::State BlockCoordinator::OnExecuteNewBlock()
{
  MilliTimer const timer{"OnExecuteNewBlock ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  new_exec_state_count_->increment();

  State next_state{State::RESET};

  // schedule the current block for execution
  if (ScheduleNextBlock())
  {
    exec_wait_periodic_.Reset();

    next_state = State::WAIT_FOR_NEW_BLOCK_EXECUTION;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnWaitForNewBlockExecution()
{
  MilliTimer const timer{"OnWaitForNewBlockExecution ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  new_wait_exec_state_count_->increment();

  State next_state{State::WAIT_FOR_NEW_BLOCK_EXECUTION};

  auto const status = QueryExecutorStatus();
  switch (status)
  {
  case ExecutionStatus::IDLE:
  {
    // update the current block with the desired hash
    next_block_->merkle_hash = storage_unit_.CurrentHash();

    FETCH_LOG_DEBUG(LOGGING_NAME, "Merkle Hash: ", ToBase64(next_block_->merkle_hash));

    // Commit the state generated by this block
    storage_unit_.Commit(next_block_->block_number);

    // Notify the DAG of this epoch
    if (dag_)
    {
      dag_->CommitEpoch(next_block_->dag_epoch);
    }

    next_state = State::TRANSMIT_BLOCK;
    break;
  }

  case ExecutionStatus::RUNNING:
    if (exec_wait_periodic_.Poll())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Waiting for new block execution (following: ",
                     next_block_->previous_hash.ToBase64(), ")");
    }

    // signal that the next execution should not happen immediately
    state_machine_->Delay(std::chrono::milliseconds{20});
    break;

  case ExecutionStatus::STALLED:
    next_state = State::RESET;
    break;
  case ExecutionStatus::ERROR:
    next_state = State::RESET;
    break;
  }

  return next_state;
}

BlockCoordinator::State BlockCoordinator::OnTransmitBlock()
{
  MilliTimer const timer{"OnTransmitBlock ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  transmit_state_count_->increment();

  try
  {
    // Before the block leaves, it must be signed.
    next_block_->UpdateDigest();
    next_block_->miner_signature = certificate_->Sign(next_block_->hash);

    FETCH_LOG_DEBUG(LOGGING_NAME, "New Block: 0x", next_block_->hash.ToHex(), " #",
                    next_block_->block_number, " Merkle: 0x", next_block_->merkle_hash.ToHex());

    // this step is needed because the execution manager is actually unaware of the actual last
    // block that is executed because the merkle hash was not known at this point.
    execution_manager_.SetLastProcessedBlock(next_block_->hash);

    // ensure that the main chain is aware of the block
    if (BlockStatus::ADDED == chain_.AddBlock(*next_block_))
    {
      // update the telemetry
      mined_block_count_->increment();
      executed_block_count_->increment();

      FETCH_LOG_INFO(LOGGING_NAME, "Broadcasting new block: 0x", next_block_->hash.ToHex(),
                     " merkle: ", next_block_->merkle_hash.ToHex(),
                     " txs: ", next_block_->GetTransactionCount(),
                     " entropy: ", next_block_->block_entropy.EntropyAsU64(),
                     " number: ", next_block_->block_number);

      // signal the last block that has been executed
      last_executed_block_.ApplyVoid([this](auto &digest) { digest = next_block_->hash; });

      // dispatch the block that has been generated
      block_sink_.OnBlock(*next_block_);

      // Metrics on block time
      total_time_to_create_block_->set(
          static_cast<uint64_t>(ToSeconds(Clock::now() - start_block_packing_)));
      blocks_minted_->add(1);
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Error transmitting verified block: ", ex.what());
  }

  return State::RESET;
}

BlockCoordinator::State BlockCoordinator::OnReset()
{
  MilliTimer const timer{"OnReset ", 1000};
  current_block_coord_state_->set(static_cast<uint64_t>(state_machine_->state()));
  Block const *block = nullptr;

  if (next_block_)
  {
    block = next_block_.get();
  }
  else if (current_block_)
  {
    block = current_block_.get();
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME,
                    "Unable to find a previously executed block! Doing a hard reset.");
    Reset();

    auto genesis_block = MainChain::CreateGenesisBlock();
    block              = genesis_block.get();
  }

  current_block_weight_->set(block->weight);
  reset_state_count_->increment();

  if (block != nullptr && !block->hash.empty())
  {
    if ((block->block_number % 100) == 0)
    {
      block_hash_->set(*reinterpret_cast<uint64_t const *>(block->hash.pointer()));
    }
  }

  try
  {
    consensus_->UpdateCurrentBlock(*block);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to update consensus with error: ", ex.what());
    consensus_update_failure_total_->increment();
  }
  catch (...)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unknown error when updating consensus!");
    consensus_update_failure_total_->increment();
  }

  current_block_.reset();
  next_block_.reset();
  pending_txs_.reset();

  return State::SYNCHRONISING;
}

bool BlockCoordinator::ScheduleCurrentBlock()
{
  bool success{false};

  // sanity check - ensure there is a block to execute
  if (current_block_)
  {
    success = ScheduleBlock(*current_block_);
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to execute empty current block");
  }

  return success;
}

bool BlockCoordinator::ScheduleNextBlock()
{
  bool success{false};

  if (next_block_)
  {
    success = ScheduleBlock(*next_block_);
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to execute empty next block");
  }

  return success;
}

bool BlockCoordinator::ScheduleBlock(Block const &block)
{
  bool success{false};

  FETCH_LOG_DEBUG(LOGGING_NAME, "Attempting exec on block: 0x", block.hash.ToHex());

  // instruct the execution manager to execute the current block
  auto const execution_status = execution_manager_.Execute(block);

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
    FETCH_LOG_WARN(LOGGING_NAME, "Execution in error state: ", ledger::ToString(execution_state));
    status = ExecutionStatus::ERROR;
    break;
  }

  return status;
}

char const *BlockCoordinator::ToString(State state)
{
  char const *text = "Unknown";

  switch (state)
  {
  case State::RELOAD_STATE:
    text = "Reloading State";
    break;
  case State::SYNCHRONISING:
    text = "Synchronising";
    break;
  case State::SYNCHRONISED:
    text = "Synchronised";
    break;
  case State::PRE_EXEC_BLOCK_VALIDATION:
    text = "Pre Block Execution Validation";
    break;
  case State::WAIT_FOR_TRANSACTIONS:
    text = "Waiting for Transactions";
    break;
  case State::SYNERGETIC_EXECUTION:
    text = "Synergetic Execution";
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
  case State::NEW_SYNERGETIC_EXECUTION:
    text = "New Synergetic Execution";
    break;
  case State::EXECUTE_NEW_BLOCK:
    text = "Execution New Block";
    break;
  case State::WAIT_FOR_NEW_BLOCK_EXECUTION:
    text = "Waiting for New Block Execution";
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

void BlockCoordinator::Reset()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Hard resetting block coordinator");
  chain_.Reset();
  current_block_ = MainChain::CreateGenesisBlock();
  last_executed_block_.ApplyVoid([](auto &digest) { digest = chain::GetGenesisDigest(); });
  execution_manager_.SetLastProcessedBlock(chain::GetGenesisDigest());
}

void BlockCoordinator::ResetGenesis()
{
  FETCH_LOG_INFO(LOGGING_NAME, "Resetting block coordinator");
  current_block_ = MainChain::CreateGenesisBlock();
  last_executed_block_.ApplyVoid([](auto &digest) { digest = chain::GetGenesisDigest(); });
  execution_manager_.SetLastProcessedBlock(chain::GetGenesisDigest());
}

}  // namespace ledger
}  // namespace fetch
