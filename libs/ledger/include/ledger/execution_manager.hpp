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
#include "ledger/chain/constants.hpp"
#include "ledger/chaincode/cache.hpp"
#include "ledger/execution_item.hpp"
#include "ledger/execution_manager_interface.hpp"
#include "ledger/executor.hpp"
#include "ledger/state_summary_archive.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "network/details/thread_pool.hpp"
#include "storage/object_store.hpp"

#include "core/byte_array/encoders.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <memory>
#include <thread>
#include <unordered_set>
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
  ExecutionManager(std::string const &storage_path, std::size_t num_executors,
                   StorageUnitPtr storage, ExecutorFactory const &factory);

  /// @name Execution Manager Interface
  /// @{
  ScheduleStatus Execute(Block const &block) override;
  BlockHash      LastProcessedBlock() override;
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
  using ExecutionItemPtr  = std::unique_ptr<ExecutionItem>;
  using ExecutionItemList = std::vector<ExecutionItemPtr>;
  using ExecutionPlan     = std::vector<ExecutionItemList>;
  using ThreadPool        = fetch::network::ThreadPool;
  using Mutex             = std::mutex;
  using Counter           = std::atomic<std::size_t>;
  using Flag              = std::atomic<bool>;
  using Bookmark          = StorageUnitInterface::bookmark_type;
  using StateHash         = StorageUnitInterface::hash_type;
  using ExecutorList      = std::vector<ExecutorPtr>;
  using StateHashCache    = storage::ObjectStore<StateHash>;
  using ThreadPtr         = std::unique_ptr<std::thread>;
  using BlockSliceList    = std::vector<chain::BlockSlice>;
  using Condition         = std::condition_variable;
  using ResourceID        = storage::ResourceID;
  using AtomicState       = std::atomic<State>;

  Flag        running_{false};
  Flag        monitor_ready_{false};
  AtomicState state_{State::IDLE};

  ChainCodeCache contracts_;
  StorageUnitPtr storage_;

  Mutex         execution_plan_lock_;  ///< guards `execution_plan_`
  ExecutionPlan execution_plan_;

  BlockHash last_block_hash_ = chain::GENESIS_DIGEST;

  Mutex     monitor_lock_;
  Condition monitor_wake_;
  Condition monitor_notify_;

  Mutex        idle_executors_lock_;  ///< guards `idle_executors`
  ExecutorList idle_executors_;

  Counter active_count_{0};
  Counter completed_executions_{0};
  Counter remaining_executions_{0};
  Counter num_slices_{0};

  ThreadPool thread_pool_;
  ThreadPtr  monitor_thread_;

  // Block state retrieval
  Mutex               state_archive_lock_;  ///< guards state_archive_ & block_state_cache_
  StateSummaryArchive state_archive_;
  StateHashCache      block_state_cache_;

  void MonitorThreadEntrypoint();

  bool PlanExecution(Block const &block);
  void DispatchExecution(ExecutionItem &item);

  bool AttemptRestoreToBlock(BlockHash const &digest);
};

}  // namespace ledger
}  // namespace fetch
