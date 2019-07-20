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

#include "core/future_timepoint.hpp"
#include "core/mutex.hpp"
#include "core/periodic_action.hpp"
#include "core/state_machine.hpp"
#include "core/threading/synchronised_state.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/dag/dag_interface.hpp"
#include "ledger/upow/naive_synergetic_miner.hpp"
#include "ledger/upow/synergetic_execution_manager_interface.hpp"
#include "ledger/upow/synergetic_miner_interface.hpp"
#include "moment/deadline_timer.hpp"
#include "telemetry/telemetry.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

namespace fetch {
namespace core {
class FeatureFlags;
}

namespace crypto {
class Prover;
}

namespace ledger {
namespace consensus {
class ConsensusMinerInterface;
}

class TransactionStatusCache;
class BlockPackerInterface;
class ExecutionManagerInterface;
class MainChain;
class StorageUnitInterface;
class BlockSinkInterface;
class StakeManagerInterface;

/**
 * The Block Coordinator is in charge of executing all the blocks that come into the system. It will
 * also decide when best to generate a new block into the network. The following diagram illustrates
 * the rough outline of the state machine.
 *
 * This diagram has been ordered into the three main categories of state, namely:
 *
 * - Catching up on a new block
 * - Mining / generating a new block
 * - Waiting in and idle / syncronised state
 *
 *                                  ┌──────────────────┐
 *                                  │   Synchronise    │
 *                                  │                  │◀───────────────────────────────┐
 *                                  └──────────────────┘                                │
 *                                            │                                         │
 *                                            │                                         │
 *                                            │                                         │
 *                  ┌─────────────────────────┴──────────────────────┐                  │
 *                  │                                                │                  │
 *                  │                                                ▼                  │
 *                  │                                      ┌──────────────────┐         │
 *                  │                                      │   Synchronised   │         │
 *                  │                         ┌────────────│                  │◀ ┐      │
 *                  │                         │            └──────────────────┘         │
 *                  │                         │                      │           │      │
 *                  │                         │                      │                  │
 *                  │                         │                      ├ ─ ─ ─ ─ ─ ┘      │
 *                  ▼                         ▼                      │                  │
 *        ┌──────────────────┐      ┌──────────────────┐             │                  │
 *        │ Pre Exec. Block  │      │  Pack New Block  │             │                  │
 *        │    Validation    │      │                  │             │                  │
 *        └──────────────────┘      └──────────────────┘             │                  │
 *                  │                         │                      │                  │
 *                  │                         │                      │                  │
 *                  ▼                         ▼                      │                  │
 *        ┌──────────────────┐      ┌──────────────────┐             │                  │
 *        │    Synergetic    │      │  New Synergetic  │             │                  │
 *        │    Execution     │      │    Execution     │             │                  │
 *        └──────────────────┘      └──────────────────┘             │                  │
 *                  │                         │                      │                  │
 *                  │                         │                      │                  │
 *                  ▼                         ▼                      │                  │
 *        ┌──────────────────┐      ┌──────────────────┐             │                  │
 *        │  Schedule Block  │      │Execute New Block │             │                  │
 *        │    Execution     │      │                  │             │                  │
 *        └──────────────────┘      └──────────────────┘             │                  │
 *                  │                         │                      │                  │
 *                  │                         │                      │                  │
 *                  ▼                         ▼                      │                  │
 *        ┌──────────────────┐      ┌──────────────────┐             │                  │
 *        │Wait for New Block│      │Wait for Execution│             │                  │
 *        │    Execution     │◀ ┐   │                  │◀ ─          │                  │
 *        └──────────────────┘      └──────────────────┘   │         │                  │
 *                  │           │             │                      │                  │
 *                  │─ ─ ─ ─ ─ ─              │─ ─ ─ ─ ─ ─ ┘         │                  │
 *                  ▼                         ▼                      │                  │
 *        ┌──────────────────┐      ┌──────────────────┐             │                  │
 *        │ Post Exec. Block │      │   Proof Search   │             │                  │
 *        │    Validation    │      │                  │◀ ─          │                  │
 *        └──────────────────┘      └──────────────────┘   │         │                  │
 *                  │                         │                      │                  │
 *                  │                         │─ ─ ─ ─ ─ ─ ┘         │                  │
 *                  │                         ▼                      │                  │
 *                  │               ┌──────────────────┐             │                  │
 *                  │               │  Transmit Block  │             │                  │
 *                  │               │                  │             │                  │
 *                  │               └──────────────────┘             │                  │
 *                  │                         │                      │                  │
 *                  └──────────────────────┐  │  ┌───────────────────┘                  │
 *                                         │  │  │                                      │
 *                                         │  │  │                                      │
 *                                         │  │  │                                      │
 *                                         ▼  ▼  ▼                                      │
 *                                  ┌──────────────────┐                                │
 *                                  │      Reset       │                                │
 *                                  │                  │────────────────────────────────┘
 *                                  └──────────────────┘
 *
 */
class BlockCoordinator
{
public:
  static constexpr char const *LOGGING_NAME = "BlockCoordinator";

  using ConstByteArray  = byte_array::ConstByteArray;
  using DAGPtr          = std::shared_ptr<ledger::DAGInterface>;
  using ProverPtr       = std::shared_ptr<crypto::Prover>;
  using StakeManagerPtr = std::shared_ptr<StakeManagerInterface>;

  enum class State
  {
    // Main loop
    RELOAD_STATE,   ///< Recovering previous state
    SYNCHRONISING,  ///< Catch up with the outstanding blocks
    SYNCHRONISED,   ///< Caught up waiting to generate a new block

    // Pipe 1
    PRE_EXEC_BLOCK_VALIDATION,  ///< Validation stage before block execution
    SYNERGETIC_EXECUTION,
    WAIT_FOR_TRANSACTIONS,       ///< Halts the state machine until all the block transactions are
                                 ///< present
    SCHEDULE_BLOCK_EXECUTION,    ///< Schedule the block to be executed
    WAIT_FOR_EXECUTION,          ///< Wait for the execution to be completed
    POST_EXEC_BLOCK_VALIDATION,  ///< Perform final block validation

    // Pipe 2
    PACK_NEW_BLOCK,  ///< Mine a new block from the head of the chain
    NEW_SYNERGETIC_EXECUTION,
    EXECUTE_NEW_BLOCK,             ///< Schedule the execution of the new block
    WAIT_FOR_NEW_BLOCK_EXECUTION,  ///< Wait for the new block to be executed
    PROOF_SEARCH,                  ///< New Block: Waiting until a hash can be found
    TRANSMIT_BLOCK,                ///< Transmit the new block to

    // Main loop
    RESET  ///< Cycle complete
  };
  using StateMachine = core::StateMachine<State>;

  static char const *ToString(State state);

  // Construction / Destruction
  BlockCoordinator(MainChain &chain, DAGPtr dag, StakeManagerPtr stake_mgr,
                   ExecutionManagerInterface &execution_manager, StorageUnitInterface &storage_unit,
                   BlockPackerInterface &packer, BlockSinkInterface &block_sink,
                   TransactionStatusCache &status_cache, core::FeatureFlags const &features,
                   ProverPtr const &prover, std::size_t num_lanes, std::size_t num_slices,
                   std::size_t block_difficulty);
  BlockCoordinator(BlockCoordinator const &) = delete;
  BlockCoordinator(BlockCoordinator &&)      = delete;
  ~BlockCoordinator()                        = default;

  template <typename R, typename P>
  void SetBlockPeriod(std::chrono::duration<R, P> const &period);
  void EnableMining(bool enable = true);
  void TriggerBlockGeneration();  // useful in tests

  std::weak_ptr<core::Runnable> GetWeakRunnable()
  {
    return state_machine_;
  }

  core::Runnable &GetRunnable()
  {
    return *state_machine_;
  }

  StateMachine const &GetStateMachine() const
  {
    return *state_machine_;
  }

  StateMachine &GetStateMachine()
  {
    return *state_machine_;
  }

  std::weak_ptr<core::StateMachineInterface> GetWeakStateMachine()
  {
    return state_machine_;
  }

  ConstByteArray GetLastExecutedBlock() const
  {
    return last_executed_block_.Get();
  }

  bool IsSynced() const
  {
    return (state_machine_->state() == State::SYNCHRONISED) &&
           (last_executed_block_.Get() == chain_.GetHeaviestBlockHash());
  }

  void Reset();

  // Operators
  BlockCoordinator &operator=(BlockCoordinator const &) = delete;
  BlockCoordinator &operator=(BlockCoordinator &&) = delete;

private:
  enum class ExecutionStatus
  {
    IDLE,
    RUNNING,
    STALLED,
    ERROR
  };

  static constexpr uint64_t COMMON_PATH_TO_ANCESTOR_LENGTH_LIMIT = 1000;

  using Mutex                = fetch::mutex::Mutex;
  using BlockPtr             = MainChain::BlockPtr;
  using NextBlockPtr         = std::unique_ptr<Block>;
  using PendingBlocks        = std::deque<BlockPtr>;
  using PendingStack         = std::vector<BlockPtr>;
  using Flag                 = std::atomic<bool>;
  using BlockPeriod          = std::chrono::milliseconds;
  using Clock                = std::chrono::system_clock;
  using Timepoint            = Clock::time_point;
  using StateMachinePtr      = std::shared_ptr<StateMachine>;
  using MinerPtr             = std::shared_ptr<consensus::ConsensusMinerInterface>;
  using TxDigestSetPtr       = std::unique_ptr<DigestSet>;
  using LastExecutedBlock    = SynchronisedState<ConstByteArray>;
  using FutureTimepoint      = fetch::core::FutureTimepoint;
  using DeadlineTimer        = fetch::moment::DeadlineTimer;
  using SynergeticExecMgrPtr = std::unique_ptr<SynergeticExecutionManagerInterface>;
  using SynExecStatus        = SynergeticExecutionManagerInterface::ExecStatus;

  /// @name Monitor State
  /// @{
  State OnReloadState();
  State OnSynchronising();
  State OnSynchronised(State current, State previous);

  // Phase 1
  State OnPreExecBlockValidation();
  State OnWaitForTransactions(State current, State previous);
  State OnSynergeticExecution();
  State OnScheduleBlockExecution();
  State OnWaitForExecution();
  State OnPostExecBlockValidation();

  // Phase 2
  State OnPackNewBlock();
  State OnNewSynergeticExecution();
  State OnExecuteNewBlock();
  State OnWaitForNewBlockExecution();
  State OnProofSearch();
  State OnTransmitBlock();
  State OnReset();
  /// @}

  bool            ScheduleCurrentBlock();
  bool            ScheduleNextBlock();
  bool            ScheduleBlock(Block const &block);
  ExecutionStatus QueryExecutorStatus();
  void            UpdateNextBlockTime();
  void            UpdateTxStatus(Block const &block);

  static char const *ToString(ExecutionStatus state);

  /// @name External Components
  /// @{
  MainChain &                chain_;              ///< Ref to system chain
  DAGPtr                     dag_;                ///< Ref to DAG
  StakeManagerPtr            stake_;              ///< Ref to Stake manager
  ExecutionManagerInterface &execution_manager_;  ///< Ref to system execution manager
  StorageUnitInterface &     storage_unit_;       ///< Ref to the storage unit
  BlockPackerInterface &     block_packer_;       ///< Ref to the block packer
  BlockSinkInterface &       block_sink_;         ///< Ref to the output sink interface
  TransactionStatusCache &   status_cache_;       ///< Ref to the tx status cache
  PeriodicAction             periodic_print_;
  MinerPtr                   miner_;
  MainChain::Blocks blocks_to_common_ancestor_;  ///< Partial vector of blocks from main chain HEAD
                                                 ///< to block coord. last executed block.
  /// @}

  /// @name Status
  /// @{
  LastExecutedBlock last_executed_block_;
  /// @}

  /// @name State Machine State
  /// @{
  Address         mining_address_;         ///< The miners address
  StateMachinePtr state_machine_;          ///< The main state machine for this service
  std::size_t     block_difficulty_;       ///< The number of leading zeros needed in the proof
  std::size_t     num_lanes_;              ///< The current number of lanes
  std::size_t     num_slices_;             ///< The current number of slices
  Flag            mining_{false};          ///< Flag to signal if this node generating blocks
  Flag            mining_enabled_{false};  ///< Short term signal to toggle on and off
  BlockPeriod     block_period_;           ///< The desired period before a block is generated
  Timepoint       next_block_time_;        ///< The next point that a block should be generated
  BlockPtr        current_block_{};        ///< The pointer to the current block (read only)
  NextBlockPtr    next_block_{};           ///< The next block being created (read / write)
  TxDigestSetPtr  pending_txs_{};          ///< The list of pending txs that are being waited on
  PeriodicAction  tx_wait_periodic_;       ///< Periodic print for transaction waiting
  PeriodicAction  exec_wait_periodic_;     ///< Periodic print for execution
  PeriodicAction  syncing_periodic_;       ///< Periodic print for synchronisation
  DeadlineTimer   wait_for_tx_timeout_{"bc:deadline"};  ///< Timeout when waiting for transactions
  DeadlineTimer   wait_before_asking_for_missing_tx_{
      "bc:deadline"};              ///< Time to wait before asking peers for any missing txs
  bool have_asked_for_missing_txs_;  ///< true if a request for missing Txs has been issued for the
                                     ///< current block
  /// @}

  /// @name Synergetic Contracts
  /// @{
  SynergeticExecMgrPtr synergetic_exec_mgr_;
  /// }

  /// @name Telemetry
  /// @{
  telemetry::CounterPtr reload_state_count_;
  telemetry::CounterPtr synchronising_state_count_;
  telemetry::CounterPtr synchronised_state_count_;
  telemetry::CounterPtr pre_valid_state_count_;
  telemetry::CounterPtr wait_tx_state_count_;
  telemetry::CounterPtr syn_exec_state_count_;
  telemetry::CounterPtr sch_block_state_count_;
  telemetry::CounterPtr wait_exec_state_count_;
  telemetry::CounterPtr post_valid_state_count_;
  telemetry::CounterPtr pack_block_state_count_;
  telemetry::CounterPtr new_syn_state_count_;
  telemetry::CounterPtr new_exec_state_count_;
  telemetry::CounterPtr new_wait_exec_state_count_;
  telemetry::CounterPtr proof_search_state_count_;
  telemetry::CounterPtr transmit_state_count_;
  telemetry::CounterPtr reset_state_count_;
  /// @}
};

template <typename R, typename P>
void BlockCoordinator::SetBlockPeriod(std::chrono::duration<R, P> const &period)
{
  using std::chrono::duration_cast;

  // convert and store
  block_period_ = duration_cast<BlockPeriod>(period);
  UpdateNextBlockTime();

  // signal that we are mining
  mining_ = true;
}

inline void BlockCoordinator::EnableMining(bool enable)
{
  mining_enabled_ = enable;
}

}  // namespace ledger
}  // namespace fetch
