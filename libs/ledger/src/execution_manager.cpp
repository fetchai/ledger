#include "ledger/execution_manager.hpp"
#include "ledger/executor.hpp"
#include "core/assert.hpp"
#include "core/logger.hpp"

#include "core/byte_array/encoders.hpp"

#include <memory>
#include <chrono>
#include <thread>

namespace fetch {
namespace ledger {

/**
 * Constructs a execution manager instance
 *
 * @param num_executors The specified number of executors (and threads)
 */
ExecutionManager::ExecutionManager(std::size_t num_executors)
  : idle_executors_(num_executors)
  , thread_pool_(std::make_shared<fetch::network::ThreadPool>(num_executors))
{
  std::lock_guard<mutex_type> lock(idle_executors_lock_);

  // ensure lists are reserved
  idle_executors_.reserve(num_executors);

  // create the executor instances
  for (std::size_t i = 0; i < num_executors; ++i)
  {
    idle_executors_.emplace_back(std::make_shared<Executor>());
  }
}

/**
 * Initiates the execution of a given block across the set of executors
 *
 * @param index The transaction index
 * @param map The map of transaction indexes
 * @param num_lanes The number of lanes
 * @param num_slices The number of slices for the block
 * @return true if successful, otherwise false
 */
bool ExecutionManager::Execute(tx_index_type const &index, block_map_type &map, std::size_t num_lanes,
                               std::size_t num_slices)
{
  static constexpr uint64_t INVALID_TRANSACTION = uint64_t(-1);
  detailed_assert(map.size() == (num_lanes * num_slices));

  if (!running_) {
    return false;
  }

  // create the execution plans
  execution_plan_type plan;

  // TODO(EJF): This should be removed and processed at a higher level (also it is very inefficient)
  for (std::size_t slice_idx = 0; slice_idx < num_slices; ++slice_idx)
  {
    for (std::size_t lane_idx = 0; lane_idx < num_lanes; ++lane_idx)
    {
      auto const tx_id = map[lane_idx * num_slices + slice_idx];

      if (tx_id == INVALID_TRANSACTION)
      {
        continue;
      }

      // determine if the transaction
      ExecutionItem item;
      item.hash = index.at(tx_id);
      item.slice = slice_idx;
      item.lanes.insert(static_cast<uint16_t>(lane_idx));

      // determine all applicable lanes
      for (std::size_t i = lane_idx + 1; i < num_lanes; ++i)
      {
        std::size_t const search_idx = i * num_lanes + slice_idx;

        // this transaction matches previous index
        if (tx_id == map[search_idx])
        {
          map[search_idx] = INVALID_TRANSACTION;
          item.lanes.insert(static_cast<uint16_t>(i));
        }
      }

      // Dispatch execution task
//      logger.Info("Dispatching transaction: ", byte_array::ToBase64(item.hash));
      auto self = shared_from_this();
      thread_pool_->Post([self, item]() {
        self->DispatchExecution(item);
      });
    }
  }

  return true;
}

/**
 * Dispatches an execution item to the next available executor
 *
 * @param item The execution item to dispatch
 */
void ExecutionManager::DispatchExecution(ExecutionItem const &item)
{
  static const std::size_t DEFAULT_WAIT_INTERVAL = 200;
  using namespace std::chrono;

//  logger.Info("Dispatching execution...");

  for (;;)
  {
    shared_executor_type executor;

    // lookup a free executor
    {
      std::lock_guard<mutex_type> lock(idle_executors_lock_);
      if (!idle_executors_.empty())
      {
        executor = idle_executors_.back();
        idle_executors_.pop_back();
      }
    }

    if (executor)
    {
      ++active_count_;
      executor->Execute(item.hash, item.slice, item.lanes);
      --active_count_;
      ++completed_executions_;

      {
        std::lock_guard<mutex_type> lock(idle_executors_lock_);
        idle_executors_.push_back(executor);
      }

      break;
    }

    // In theory this should never be called since the number of executors is
    // matched to the number of threads.
    std::this_thread::sleep_for(milliseconds(DEFAULT_WAIT_INTERVAL));
  }
}

/**
 * Starts the execution manager running
 */
void ExecutionManager::Start()
{
  running_ = true;
  thread_pool_->Start();
}

/**
 * Stops the execution manager running
 */
void ExecutionManager::Stop()
{
  running_ = false;
  thread_pool_->Stop();
}

} // namespace ledger
} // namespace fetch
