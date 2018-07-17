#include "ledger/execution_manager.hpp"
#include "ledger/executor.hpp"
#include "core/assert.hpp"
#include "core/logger.hpp"

#include "core/byte_array/encoders.hpp"

#include <memory>
#include <chrono>
#include <thread>
#include <vector>

#include <iostream>

namespace fetch {
namespace ledger {

/**
 * Constructs a execution manager instance
 *
 * @param num_executors The specified number of executors (and threads)
 */
// std::make_shared<fetch::network::ThreadPool>(num_executors)
ExecutionManager::ExecutionManager(std::size_t num_executors, storage_unit_type storage, executor_factory_type const &factory)
  : storage_(std::move(storage))
  , idle_executors_(num_executors)
  , thread_pool_(network::MakeThreadPool(num_executors)) {

  // setup the executor pool
  {
    std::lock_guard<mutex_type> lock(idle_executors_lock_);

    // ensure lists are reserved
    idle_executors_.reserve(num_executors);

    // create the executor instances
    for (std::size_t i = 0; i < num_executors; ++i) {
      idle_executors_.emplace_back(factory());
    }
  }
}

/**
 * Initiates the execution of a given block across the set of executors
 *
 * @param block_hash The hash of the current block to process
 * @param prev_block_hash The prev hash of the current block to process
 * @param index The transaction index
 * @param map The map of transaction indexes
 * @param num_lanes The number of lanes
 * @param num_slices The number of slices for the block
 * @return true if successful, otherwise false
 */
bool ExecutionManager::Execute(block_digest_type const &block_hash,
                               block_digest_type const &prev_block_hash,
                               tx_index_type const &index,
                               block_map_type &map,
                               std::size_t num_lanes,
                               std::size_t num_slices) {

  detailed_assert(map.size() == (num_lanes * num_slices));

  // if the execution manager is not running then no further transactions
  // should be scheduled
  if (!running_) {
    return false;
  }

  // if a block is currently being processed we should not allow another block
  // to be processed
  if (IsActive()) {
    return false;
  }

  // determine if the current block follows on from the previous block
  if (prev_block_hash != last_block_hash_) {
    std::lock_guard<mutex_type> lock(state_archive_lock_);

    // need to load the state from a previous application, so lookup the corresponding state hash
    auto cache_it = block_state_cache_.find(prev_block_hash);
    if (cache_it == block_state_cache_.end()) {
      logger.Warn("Unable to lookup previous state hash");
      return false;
    }

    // lookup the cached bookmark
    bookmark_type bookmark{0};
    if (!state_archive_.LookupBookmark(cache_it->second, bookmark)) {
      logger.Warn("Unable to lookup previous state hash bookmark");
      return false;
    }

    // revert the storage engine to the previous bookmark
    storage_->Revert(bookmark);
  }

  // sanity check!
  detailed_assert(IsIdle());

  // TODO: (EJF) Detect and handle number of lanes updates

  // plan the execution for this block
  PlanExecution(index, map, num_lanes, num_slices);

  // update the last block hash
  last_block_hash_ = block_hash;
  num_slices_ = num_slices;

  // trigger the monitor / dispatch thread
  monitor_wake_.notify_one();

  return true;
}

void ExecutionManager::PlanExecution(tx_index_type const &index, block_map_type &map, std::size_t num_lanes,
                                     std::size_t num_slices) {

  static constexpr uint64_t INVALID_TRANSACTION = uint64_t(-1);
  using lane_index_type = ExecutionItem::lane_index_type;

  std::lock_guard<mutex_type> lock(execution_plan_lock_);

  // clear and resize the execution plan
  execution_plan_.clear();
  execution_plan_.resize(num_slices);

  // TODO(EJF): This should be removed and processed at a higher level (also it is inefficient)
  for (std::size_t slice_idx = 0; slice_idx < num_slices; ++slice_idx) {
    for (lane_index_type lane_idx = 0; lane_idx < num_lanes; ++lane_idx) {
      auto const tx_id = map[lane_idx * num_slices + slice_idx];

      if (tx_id == INVALID_TRANSACTION) {
        continue;
      }

      // determine if the transaction
      auto item = fetch::make_unique<ExecutionItem>(index.at(tx_id), lane_idx, slice_idx);

      // determine all applicable lanes
      for (std::size_t i = lane_idx + 1; i < num_lanes; ++i) {
        std::size_t const search_idx = i * num_slices + slice_idx;

        // this transaction matches previous index
        if (tx_id == map[search_idx]) {
          map[search_idx] = INVALID_TRANSACTION;
          item->AddLane(static_cast<uint16_t>(i));
        }
      }

      // update the execution plan
      execution_plan_[slice_idx].emplace_back(std::move(item));
    }
  }
}

/**
 * Dispatches an execution item to the next available executor
 *
 * This function should be called from a context of a thread pool
 *
 * @param item The execution item to dispatch
 */
void ExecutionManager::DispatchExecution(ExecutionItem &item) {
  shared_executor_type executor;

  // lookup a free executor
  {
    std::lock_guard<mutex_type> lock(idle_executors_lock_);
    if (!idle_executors_.empty()) {
      executor = idle_executors_.back();
      idle_executors_.pop_back();
    }
  }

  // We must have a executor present for this to work. This should always
  // be the case provided num_executors == num_threads (in thread pool)
  detailed_assert(executor);

  if (executor) {

    ++active_count_;
    auto status = item.Execute(*executor);
    (void) status;
    // TODO: (EJF) Should work out what the status of this is
    --active_count_;
    --remaining_executions_;
    ++completed_executions_;

    {
      std::lock_guard<mutex_type> lock(idle_executors_lock_);
      idle_executors_.push_back(executor);
    }

    // trigger the monitor thread to process the next slice if needed
    monitor_notify_.notify_one();
  }
}

/**
 * Starts the execution manager running
 */
void ExecutionManager::Start() {
  running_ = true;

  // create and start the monitor thread
  monitor_thread_ = fetch::make_unique<std::thread>(&ExecutionManager::MonitorThreadEntrypoint, this);

  // wait for the monitor thread to be setup
  for (std::size_t i = 0; i < 20; ++i) {
    if (monitor_ready_)
      break;

    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

  if (!monitor_ready_) {
    throw std::runtime_error("Failed waiting for the monitor to start");
  }

  // fireup the main worker thread pool
  thread_pool_->Start();
}

/**
 * Stops the execution manager running
 */
void ExecutionManager::Stop() {
  running_ = false;

  // trigger the monitor thread to wake up (so that it sees the change in running_)
  monitor_wake_.notify_one();
  monitor_notify_.notify_one();

  // wait for the monitor thread to exit
  monitor_thread_->join();
  monitor_thread_.reset();

  // tear down the thread pool
  thread_pool_->Stop();
}

ExecutionManagerInterface::block_digest_type ExecutionManager::LastProcessedBlock() {
  // TODO: (EJF) thread saftey
  return last_block_hash_;
}

bool ExecutionManager::IsActive() {
  return active_;
}

bool ExecutionManager::IsIdle() {
  return !active_;
}

bool ExecutionManager::Abort() {
  return false;
}

void ExecutionManager::MonitorThreadEntrypoint() {
  using namespace std::chrono;

  enum class MonitorState {
    IDLE,
    SCHEDULE_NEXT_SLICE,
    RUNNING,
    BOOKMARKING_STATE
  };

  MonitorState state = MonitorState::IDLE;

  std::size_t next_slice = 0;
  std::mutex wait_lock;
  block_digest_type current_block;

  while (running_) {
    monitor_ready_ = true;

    switch (state) {
      case MonitorState::IDLE: {
        active_ = false;

        // enter the idle state where we wait for the next block to be posted
        {
          std::unique_lock<std::mutex> lock(wait_lock);
          monitor_wake_.wait(lock);
        }

        active_ = true;
        current_block = last_block_hash_;

        // schedule the next slice if we have been triggered
        if (running_) {
          state = MonitorState::SCHEDULE_NEXT_SLICE;
          next_slice = 0;
        }

        break;
      }

      case MonitorState::SCHEDULE_NEXT_SLICE: {
        std::lock_guard<mutex_type> lock(execution_plan_lock_);

        auto const &slice_plan = execution_plan_[next_slice];

        // determine the target number of executions being expected (must be done before the
        // thread pool dispatch)
        remaining_executions_ = slice_plan.size();

        for (auto &item : slice_plan) {
          // create the closure and dispatch to the thread pool
          auto self = shared_from_this();
          thread_pool_->Post([self, &item]() {
            self->DispatchExecution(*item);
          });
        }

        state = MonitorState::RUNNING;
        ++next_slice;
        break;
      }

      case MonitorState::RUNNING: {

        // wait for the execution to complete
        if (remaining_executions_ > 0) {
          std::unique_lock<std::mutex> lock(wait_lock);
          monitor_notify_.wait_for(lock, milliseconds{100});
        }

        // determine the next state (provided we have comlete
        if (remaining_executions_ == 0) {
          state = (num_slices_ > next_slice) ? MonitorState::SCHEDULE_NEXT_SLICE : MonitorState::BOOKMARKING_STATE;
        }

        break;
      }

      case MonitorState::BOOKMARKING_STATE: {

        // lookup the final hash
        hash_type state_hash = storage_->Hash();

        // request a bookmark
        bookmark_type bookmark = 0;
        bool new_bookmark = false;
        if (state_hash.size()) {
          std::lock_guard<mutex_type> lock(state_archive_lock_);
          new_bookmark = state_archive_.RecordBookmark(state_hash, bookmark);
        } else {
          logger.Warn("Unable to request state hash");
        }

        // only need to commit the new bookmark if there is actually a change in state
        if (new_bookmark) {

          // commit the changes in state
          try {
            storage_->Commit(bookmark);
          } catch (std::exception &ex) {
            logger.Warn("Unable to commit state. Error: ", ex.what());
          }

          // update the state archives
          {
            std::lock_guard<mutex_type> lock(state_archive_lock_);
            if (!state_archive_.ConfirmBookmark(state_hash, bookmark)) {
              logger.Warn("Unable to confirm bookmark: ", bookmark);
            }

            // update the block state cache
            block_state_cache_.emplace(current_block, state_hash);
          }
        }

        // finished processing the block
        state = MonitorState::IDLE;

        break;
      }
    }
  }
}

} // namespace ledger
} // namespace fetch
