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

#include "core/byte_array/encoders.hpp"
#include "core/mutex.hpp"
#include "core/threading/synchronised_state.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/chain/constants.hpp"
#include "ledger/execution_item.hpp"
#include "ledger/execution_manager_interface.hpp"
#include "ledger/executor.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "network/details/thread_pool.hpp"
#include "storage/object_store.hpp"
#include "telemetry/telemetry.hpp"
#include "transaction_status_cache.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace fetch {
namespace ledger {

/**
 * The Execution Manager is the object which orchestrates the execution of a
 * specified block across a series of executors and lanes.
 */
class ExecutionManager : public ExecutionManagerInterface,
                         public std::enable_shared_from_this<ExecutionManager>
{
public:
  using StorageUnitPtr  = std::shared_ptr<StorageUnitInterface>;
  using ExecutorPtr     = std::shared_ptr<ExecutorInterface>;
  using ExecutorFactory = std::function<ExecutorPtr()>;

  // Construction / Destruction
  ExecutionManager(std::size_t num_executors, uint32_t log2_num_lanes, StorageUnitPtr storage,
                   ExecutorFactory const &factory, TransactionStatusCache::ShrdPtr tx_status_cache);

  /// @name Execution Manager Interface
  /// @{
  ScheduleStatus Execute(Block::Body const &block) override;
  void           SetLastProcessedBlock(Digest digest) override;
  Digest         LastProcessedBlock() override;
  State          GetState() override;
  bool           Abort() override;
  /// @}

  // general control of the operation of the module
  void Start();
  void Stop();

  // statistics
  std::size_t completed_executions() const
  {
    return completed_executions_;
  }

private:
  struct Counters
  {
    std::size_t active{0};
    std::size_t remaining{0};
  };

  using ExecutionItemPtr  = std::unique_ptr<ExecutionItem>;
  using ExecutionItemList = std::vector<ExecutionItemPtr>;
  using ExecutionPlan     = std::vector<ExecutionItemList>;
  using ThreadPool        = fetch::network::ThreadPool;
  using Mutex             = std::mutex;
  using Counter           = std::atomic<std::size_t>;
  using Flag              = std::atomic<bool>;
  using StateHash         = StorageUnitInterface::Hash;
  using ExecutorList      = std::vector<ExecutorPtr>;
  using StateHashCache    = storage::ObjectStore<StateHash>;
  using ThreadPtr         = std::unique_ptr<std::thread>;
  using BlockSliceList    = ledger::Block::Slices;
  using Condition         = std::condition_variable;
  using ResourceID        = storage::ResourceID;
  using AtomicState       = std::atomic<State>;
  using SyncCounters      = SynchronisedState<Counters>;
  using SyncedState       = SynchronisedState<State>;
  using CounterPtr        = telemetry::CounterPtr;
  using HistogramPtr      = telemetry::HistogramPtr;

  uint32_t const log2_num_lanes_;

  Flag running_{false};
  Flag monitor_ready_{false};

  SyncedState state_{State::IDLE};

  StorageUnitPtr storage_;

  Mutex         execution_plan_lock_;  ///< guards `execution_plan_`
  ExecutionPlan execution_plan_;

  Digest  last_block_hash_ = GENESIS_DIGEST;
  Address last_block_miner_{};

  Mutex     monitor_lock_;
  Condition monitor_wake_;
  Condition monitor_notify_;

  Mutex        idle_executors_lock_;  ///< guards `idle_executors`
  ExecutorList idle_executors_;

  Counter completed_executions_{0};
  Counter num_slices_{0};

  SyncCounters counters_{};

  ThreadPool thread_pool_;
  ThreadPtr  monitor_thread_;

  TransactionStatusCache::ShrdPtr tx_status_cache_;  ///< Ref to the tx status cache
  // Telemetry
  CounterPtr   tx_executed_count_;
  CounterPtr   slices_executed_count_;
  CounterPtr   fees_settled_count_;
  CounterPtr   blocks_completed_count_;
  HistogramPtr execution_duration_;

  void MonitorThreadEntrypoint();

  bool PlanExecution(Block::Body const &block);
  void DispatchExecution(ExecutionItem &item);
};

}  // namespace ledger
}  // namespace fetch
