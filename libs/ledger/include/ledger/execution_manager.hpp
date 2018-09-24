#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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
  using shared_executor_type = std::shared_ptr<ExecutorInterface>;
  using executor_list_type   = std::vector<shared_executor_type>;
  using block_slices_type    = std::vector<chain::BlockSlice>;
  using block_digest_type    = byte_array::ConstByteArray;
  using contract_cache_type  = ChainCodeCache;

  using execution_item_type = std::unique_ptr<ExecutionItem>;
  using execution_list_type = std::vector<execution_item_type>;
  using execution_plan_type = std::vector<execution_list_type>;

  using thread_pool_type       = fetch::network::ThreadPool;
  using mutex_type             = std::mutex;
  using counter_type           = std::atomic<std::size_t>;
  using flag_type              = std::atomic<bool>;
  using thread_type            = std::unique_ptr<std::thread>;
  using executor_factory_type  = std::function<shared_executor_type()>;
  using storage_unit_type      = std::shared_ptr<StorageUnitInterface>;
  using bookmark_type          = StorageUnitInterface::bookmark_type;
  using hash_type              = StorageUnitInterface::hash_type;
  using block_state_cache_type = std::unordered_map<block_digest_type, hash_type>;

  // Construction / Destruction
  explicit ExecutionManager(std::size_t num_executors, storage_unit_type storage,
                            executor_factory_type const &factory);

  /// @name Execution Manager Interface
  /// @{
  Status            Execute(block_type const &block) override;
  block_digest_type LastProcessedBlock() override;
  bool              IsActive() override;
  bool              IsIdle() override;
  bool              Abort() override;
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
  flag_type running_{false};
  flag_type active_{false};
  flag_type monitor_ready_{false};

  contract_cache_type contracts_;
  storage_unit_type   storage_;
  execution_plan_type execution_plan_;
  mutex_type          execution_plan_lock_;
  block_digest_type   last_block_hash_ = chain::GENESIS_DIGEST;

  std::condition_variable monitor_wake_;
  std::condition_variable monitor_notify_;

  mutex_type         idle_executors_lock_;
  executor_list_type idle_executors_;

  counter_type     active_count_{0};
  counter_type     completed_executions_{0};
  counter_type     remaining_executions_{0};
  thread_pool_type thread_pool_;
  thread_type      monitor_thread_;

  // std::size_t slice_index_{0};
  std::atomic<std::size_t> num_slices_;

  mutex_type state_archive_lock_;  ///< guards both the state_archive_ and the
                                   ///< block_state_cache_
  StateSummaryArchive    state_archive_;
  block_state_cache_type block_state_cache_;  // TODO(issue 33): Both these caches require
                                              // maintenance to stop them growing forever

  void MonitorThreadEntrypoint();

  bool PlanExecution(block_type const &block);
  void DispatchExecution(ExecutionItem &item);

  bool AttemptRestoreToBlock(block_digest_type const &digest);
};

}  // namespace ledger
}  // namespace fetch
