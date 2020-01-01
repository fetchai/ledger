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

#include "chain/address.hpp"
#include "chain/constants.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/mutex.hpp"
#include "core/synchronisation/protected.hpp"
#include "core/synchronisation/waitable.hpp"
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
  ScheduleStatus Execute(Block const &block) override;
  void           SetLastProcessedBlock(Digest hash) override;
  Digest         LastProcessedBlock() const override;
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
  using CounterPtr        = telemetry::CounterPtr;
  using HistogramPtr      = telemetry::HistogramPtr;
  using BlockIndex        = uint64_t;

  struct Summary
  {
    State          state{State::IDLE};
    Digest         last_block_hash{chain::ZERO_HASH};
    BlockIndex     last_block_number{0};
    chain::Address last_block_miner{};
  };

  uint32_t const log2_num_lanes_;

  Flag running_{false};
  Flag monitor_ready_{false};

  Protected<Summary> state_{};

  StorageUnitPtr storage_;

  Mutex         execution_plan_lock_;  ///< guards `execution_plan_`
  ExecutionPlan execution_plan_;

  Mutex     monitor_lock_;
  Condition monitor_wake_;
  Condition monitor_notify_;

  Mutex        idle_executors_lock_;  ///< guards `idle_executors`
  ExecutorList idle_executors_;

  Counter completed_executions_{0};
  Counter num_slices_{0};

  Waitable<Counters> counters_{};

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

  bool PlanExecution(Block const &block);
  void DispatchExecution(ExecutionItem &item);
};

}  // namespace ledger
}  // namespace fetch
