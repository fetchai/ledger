#pragma once
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

#include "chain/transaction.hpp"
#include "core/future_timepoint.hpp"
#include "core/mutex.hpp"
#include "core/periodic_action.hpp"
#include "core/state_machine.hpp"
#include "core/synchronisation/protected.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"
#include "ledger/consensus/consensus_interface.hpp"
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

class TransactionStatusCache;
class BlockPackerInterface;
class ExecutionManagerInterface;
class MainChain;
class StorageUnitInterface;
class BlockSinkInterface;

/**
 * The Block Coordinator is in charge of executing all the blocks that come into the system. It will
 * also decide when best to generate a new block into the network. The following diagram illustrates
 * the rough outline of the state machine.
 *
 * This diagram has been ordered into the three main categories of state, namely:
 *
 * - Catching up on a new block
 * - Mining / generating a new block
 * - Waiting in and idle / synchronised state
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
 *        │ Post Exec. Block │      │                  │             │                  │
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

  using ConstByteArray = byte_array::ConstByteArray;
  using DAGPtr         = std::shared_ptr<ledger::DAGInterface>;
  using ProverPtr      = std::shared_ptr<crypto::Prover>;
  using ConsensusPtr   = std::shared_ptr<ledger::ConsensusInterface>;

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
    TRANSMIT_BLOCK,                ///< Transmit the new block to

    // Main loop
    RESET  ///< Cycle complete
  };
  using StateMachine         = core::StateMachine<State>;
  using SynergeticExecMgrPtr = std::unique_ptr<SynergeticExecutionManagerInterface>;

  static char const *ToString(State state);

  // Construction / Destruction
  BlockCoordinator(MainChain &chain, DAGPtr dag, ExecutionManagerInterface &execution_manager,
                   StorageUnitInterface &storage_unit, BlockPackerInterface &packer,
                   BlockSinkInterface &block_sink, ProverPtr prover, uint32_t log2_num_lanes,
                   std::size_t num_slices, ConsensusPtr consensus,
                   SynergeticExecMgrPtr synergetic_exec_manager);
  BlockCoordinator(BlockCoordinator const &) = delete;
  BlockCoordinator(BlockCoordinator &&)      = delete;
  ~BlockCoordinator()                        = default;

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
    return last_executed_block_.Apply([](auto const &last_executed_block_hash) -> ConstByteArray {
      return last_executed_block_hash;
    });
  }

  bool IsSynced() const
  {
    return last_executed_block_.Apply([this](auto const &last_executed_block_hash) -> bool {
      return (state_machine_->state() == State::SYNCHRONISED) &&
             (last_executed_block_hash == chain_.GetHeaviestBlockHash());
    });
  }

  void Reset();
  void ResetGenesis();

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

  static constexpr uint64_t COMMON_PATH_TO_ANCESTOR_LENGTH_LIMIT = 5000;

  using BlockPtr          = MainChain::BlockPtr;
  using NextBlockPtr      = std::unique_ptr<Block>;
  using PendingBlocks     = std::deque<BlockPtr>;
  using PendingStack      = std::vector<BlockPtr>;
  using Flag              = std::atomic<bool>;
  using BlockPeriod       = std::chrono::milliseconds;
  using Clock             = std::chrono::system_clock;
  using Timepoint         = Clock::time_point;
  using StateMachinePtr   = std::shared_ptr<StateMachine>;
  using TxDigestSetPtr    = std::unique_ptr<DigestSet>;
  using LastExecutedBlock = Protected<ConstByteArray>;
  using FutureTimepoint   = fetch::core::FutureTimepoint;
  using DeadlineTimer     = fetch::moment::DeadlineTimer;
  using SynExecStatus     = SynergeticExecutionManagerInterface::ExecStatus;

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
  State OnTransmitBlock();
  State OnReset();
  /// @}

  bool            ScheduleCurrentBlock();
  bool            ScheduleNextBlock();
  bool            ScheduleBlock(Block const &block);
  ExecutionStatus QueryExecutorStatus();
  void            RemoveBlock(MainChain::BlockHash const &hash);

  static char const *ToString(ExecutionStatus state);

  /// @name External Components
  /// @{
  MainChain &                chain_;  ///< Ref to system chain
  DAGPtr                     dag_;    ///< Ref to DAG
  ConsensusPtr               consensus_;
  ExecutionManagerInterface &execution_manager_;  ///< Ref to system execution manager
  StorageUnitInterface &     storage_unit_;       ///< Ref to the storage unit
  BlockPackerInterface &     block_packer_;       ///< Ref to the block packer
  BlockSinkInterface &       block_sink_;         ///< Ref to the output sink interface
  PeriodicAction             periodic_print_;
  MainChain::Blocks blocks_to_common_ancestor_;  ///< Partial vector of blocks from main chain HEAD
                                                 ///< to block coord. last executed block.
  /// @}

  /// @name Status
  /// @{
  LastExecutedBlock last_executed_block_;
  /// @}

  /// @name State Machine State
  /// @{
  ProverPtr       certificate_;     ///< The miners identity
  chain::Address  mining_address_;  ///< The miners address
  StateMachinePtr state_machine_;   ///< The main state machine for this service
  uint32_t        log2_num_lanes_{};
  std::size_t     num_lanes_{1u << log2_num_lanes_};  ///< The current number of lanes
  std::size_t     num_slices_;                        ///< The current number of slices
  BlockPtr        current_block_{};         ///< The pointer to the current block (read only)
  NextBlockPtr    next_block_{};            ///< The next block being created (read / write)
  TxDigestSetPtr  pending_txs_{};           ///< The list of pending txs that are being waited on
  PeriodicAction  tx_wait_periodic_;        ///< Periodic print for transaction waiting
  PeriodicAction  exec_wait_periodic_;      ///< Periodic print for execution
  PeriodicAction  syncing_periodic_;        ///< Periodic print for synchronisation
  Timepoint       start_waiting_for_tx_{};  ///< The time at which we started waiting for txs
  Timepoint       start_block_packing_{};   ///< The time at which we started block packing
  /// Timeout when waiting for transactions
  DeadlineTimer wait_for_tx_timeout_{"bc:deadline"};
  /// Time to wait before asking peers for any missing txs
  DeadlineTimer wait_before_asking_for_missing_tx_{"bc:deadline"};
  /// true if a request for missing Txs has been issued for the current block
  bool have_asked_for_missing_txs_{};
  /// @}

  /// @name Synergetic Contracts
  /// @{
  SynergeticExecMgrPtr synergetic_exec_mgr_;
  /// }

  /// @name Telemetry
  /// @{
  telemetry::CounterPtr         reload_state_count_;
  telemetry::CounterPtr         synchronising_state_count_;
  telemetry::CounterPtr         synchronised_state_count_;
  telemetry::CounterPtr         pre_valid_state_count_;
  telemetry::CounterPtr         wait_tx_state_count_;
  telemetry::CounterPtr         syn_exec_state_count_;
  telemetry::CounterPtr         sch_block_state_count_;
  telemetry::CounterPtr         wait_exec_state_count_;
  telemetry::CounterPtr         post_valid_state_count_;
  telemetry::CounterPtr         pack_block_state_count_;
  telemetry::CounterPtr         new_syn_state_count_;
  telemetry::CounterPtr         new_exec_state_count_;
  telemetry::CounterPtr         new_wait_exec_state_count_;
  telemetry::CounterPtr         transmit_state_count_;
  telemetry::CounterPtr         reset_state_count_;
  telemetry::CounterPtr         executed_block_count_;
  telemetry::CounterPtr         mined_block_count_;
  telemetry::CounterPtr         executed_tx_count_;
  telemetry::CounterPtr         request_tx_count_;
  telemetry::CounterPtr         unable_to_find_tx_count_;
  telemetry::CounterPtr         blocks_minted_;
  telemetry::CounterPtr         consensus_update_failure_total_;
  telemetry::HistogramPtr       tx_sync_times_;
  telemetry::GaugePtr<uint64_t> current_block_num_;
  telemetry::GaugePtr<uint64_t> next_block_num_;
  telemetry::GaugePtr<uint64_t> block_hash_;
  telemetry::GaugePtr<uint64_t> total_time_to_create_block_;
  telemetry::GaugePtr<uint64_t> current_block_weight_;
  telemetry::GaugePtr<uint64_t> last_block_interval_s_;
  telemetry::GaugePtr<uint64_t> current_block_coord_state_;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
