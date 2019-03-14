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

#include "core/mutex.hpp"
#include "core/periodic_action.hpp"
#include "core/state_machine.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chain/main_chain.hpp"

#include <atomic>
#include <chrono>
#include <deque>
#include <thread>
#include <unordered_set>

namespace fetch {
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
 *                           ┌──────────────────┐
 *                           │   Synchronise    │
 *                           │                  │◀───────────────────────────────┐
 *                           └──────────────────┘                                │
 *                                     │                                         │
 *                                     │                                         │
 *                                     │                                         │
 *           ┌─────────────────────────┴────────────────────────┐                │
 *           │                                                  │                │
 *           │                                                  ▼                │
 *           │                                        ┌──────────────────┐       │
 *           │                                        │   Synchronised   │       │
 *           │                         ┌──────────────│                  │◀ ┐    │
 *           │                         │              └──────────────────┘       │
 *           │                         │                        │           │    │
 *           │                         │                        │                │
 *           │                         │                        ├ ─ ─ ─ ─ ─ ┘    │
 *           ▼                         ▼                        │                │
 * ┌──────────────────┐      ┌──────────────────┐               │                │
 * │ Pre Exec. Block  │      │  Pack New Block  │               │                │
 * │    Validation    │      │                  │               │                │
 * └──────────────────┘      └──────────────────┘               │                │
 *           │                         │                        │                │
 *           │                         │                        │                │
 *           ▼                         ▼                        │                │
 * ┌──────────────────┐      ┌──────────────────┐               │                │
 * │  Schedule Block  │      │Execute New Block │               │                │
 * │    Execution     │      │                  │               │                │
 * └──────────────────┘      └──────────────────┘               │                │
 *           │                         │                        │                │
 *           │                         │                        │                │
 *           ▼                         ▼                        │                │
 * ┌──────────────────┐      ┌──────────────────┐               │                │
 * │Wait for New Block│      │Wait for Execution│               │                │
 * │    Execution     │◀ ─   │                  │◀ ─            │                │
 * └──────────────────┘   │  └──────────────────┘   │           │                │
 *           │                         │                        │                │
 *           │─ ─ ─ ─ ─ ─ ┘            │─ ─ ─ ─ ─ ─ ┘           │                │
 *           ▼                         ▼                        │                │
 * ┌──────────────────┐      ┌──────────────────┐               │                │
 * │ Post Exec. Block │      │   Proof Search   │               │                │
 * │    Validation    │      │                  │◀ ─            │                │
 * └──────────────────┘      └──────────────────┘   │           │                │
 *           │                         │                        │                │
 *           │                         │─ ─ ─ ─ ─ ─ ┘           │                │
 *           │                         ▼                        │                │
 *           │               ┌──────────────────┐               │                │
 *           │               │  Transmit Block  │               │                │
 *           │               │                  │               │                │
 *           │               └──────────────────┘               │                │
 *           │                         │                        │                │
 *           └─────────────────────────┼────────────────────────┘                │
 *                                     │                                         │
 *                                     │                                         │
 *                                     │                                         │
 *                                     ▼                                         │
 *                           ┌──────────────────┐                                │
 *                           │      Reset       │                                │
 *                           │                  │────────────────────────────────┘
 *                           └──────────────────┘
 *
 */
class BlockCoordinator
{
public:
  static constexpr char const *LOGGING_NAME = "BlockCoordinator";

  using Identity = byte_array::ConstByteArray;

  enum class State
  {
    SYNCHRONIZING,                 ///< Catch up with the outstanding blocks
    SYNCHRONIZED,                  ///< Caught up waiting to generate a new block
    PRE_EXEC_BLOCK_VALIDATION,     ///< Validation stage before block execution
    WAIT_FOR_TRANSACTIONS,         ///< Halts the state machine until all the block transactions are
                                   ///< present
    SCHEDULE_BLOCK_EXECUTION,      ///< Schedule the block to be executed
    WAIT_FOR_EXECUTION,            ///< Wait for the execution to be completed
    POST_EXEC_BLOCK_VALIDATION,    ///< Perform final block validation
    PACK_NEW_BLOCK,                ///< Mine a new block from the head of the chain
    EXECUTE_NEW_BLOCK,             ///< Schedule the execution of the new block
    WAIT_FOR_NEW_BLOCK_EXECUTION,  ///< Wait for the new block to be executed
    PROOF_SEARCH,                  ///< New Block: Waiting until a hash can be found
    TRANSMIT_BLOCK,                ///< Transmit the new block to
    RESET                          ///< Cycle complete
  };
  using StateMachine = core::StateMachine<State>;

  // Construction / Destruction
  BlockCoordinator(MainChain &chain, ExecutionManagerInterface &execution_manager,
                   StorageUnitInterface &storage_unit, BlockPackerInterface &packer,
                   BlockSinkInterface &block_sink, TransactionStatusCache &status_cache,
                   Identity identity, std::size_t num_lanes, std::size_t num_slices,
                   std::size_t block_difficulty);
  BlockCoordinator(BlockCoordinator const &) = delete;
  BlockCoordinator(BlockCoordinator &&)      = delete;
  ~BlockCoordinator();

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

  std::weak_ptr<core::StateMachineInterface> GetWeakStateMachine()
  {
    return state_machine_;
  }

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

  //  using Super         = core::StateMachine<BlockCoordinatorState>;
  using Mutex           = fetch::mutex::Mutex;
  using BlockPtr        = MainChain::BlockPtr;
  using NextBlockPtr    = std::unique_ptr<Block>;
  using PendingBlocks   = std::deque<BlockPtr>;
  using PendingStack    = std::vector<BlockPtr>;
  using Flag            = std::atomic<bool>;
  using BlockPeriod     = std::chrono::milliseconds;
  using Clock           = std::chrono::system_clock;
  using Timepoint       = Clock::time_point;
  using StateMachinePtr = std::shared_ptr<StateMachine>;
  using MinerPtr        = std::shared_ptr<consensus::ConsensusMinerInterface>;
  using TxSet           = std::unordered_set<TransactionSummary::TxDigest>;
  using TxSetPtr        = std::unique_ptr<TxSet>;

  /// @name Monitor State
  /// @{
  State OnSynchronizing();
  State OnSynchronized(State current, State previous);
  State OnPreExecBlockValidation();
  State OnWaitForTransactions();
  State OnScheduleBlockExecution();
  State OnWaitForExecution();
  State OnPostExecBlockValidation();
  State OnPackNewBlock();
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

  static char const *ToString(State state);
  static char const *ToString(ExecutionStatus state);

  /// @name External Components
  /// @{
  MainChain &                chain_;              ///< Ref to system chain
  ExecutionManagerInterface &execution_manager_;  ///< Ref to system execution manager
  StorageUnitInterface &     storage_unit_;       ///< Ref to the storage unit
  BlockPackerInterface &     block_packer_;       ///< Ref to the block packer
  BlockSinkInterface &       block_sink_;         ///< Ref to the output sink interface
  TransactionStatusCache &   status_cache_;       ///< Ref to the tx status cache
  PeriodicAction             periodic_print_;
  MinerPtr                   miner_;
  /// @}

  /// @name State Machine State
  /// @{
  Identity        identity_{};             ///< The miner identity
  StateMachinePtr state_machine_;          ///< The main state machine for this service
  std::size_t     block_difficulty_;       ///< The number of leading zeros needed in the proof
  std::size_t     num_lanes_;              ///< The current number of lanes
  std::size_t     num_slices_;             ///< The current number of slices
  std::size_t     stall_count_{0};         ///< The number of times the execution has been stalled
  Flag            mining_{false};          ///< Flag to signal if this node generating blocks
  Flag            mining_enabled_{false};  ///< Short term signal to toggle on and off
  BlockPeriod     block_period_;           ///< The desired period before a block is generated
  Timepoint       next_block_time_;        ///< THe next point that a block should be generated
  BlockPtr        current_block_{};        ///< The pointer to the current block (read only)
  NextBlockPtr    next_block_{};           ///< The next block being created (read / write)
  TxSetPtr        pending_txs_{};          ///< The list of pending txs that are being waited on
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
