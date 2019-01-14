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

#include "ledger/execution_manager.hpp"
#include "core/assert.hpp"
#include "core/logger.hpp"
#include "ledger/executor.hpp"
#include "storage/resource_mapper.hpp"

#include "core/byte_array/encoders.hpp"

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <iostream>

static constexpr char const *LOGGING_NAME              = "ExecutionManager";
static constexpr std::size_t MAX_STARTUP_ITERATIONS    = 20;
static constexpr std::size_t STARTUP_ITERATION_TIME_MS = 100;

namespace {

// TODO(HUT): consider moving/refactoring this
struct FilePaths
{
  std::string data_path;
  std::string index_path;

  static FilePaths Create(std::string const &prefix, std::string const &basename)
  {
    FilePaths f;
    f.data_path  = prefix + basename + ".db";
    f.index_path = prefix + basename + ".index.db";

    return f;
  }
};

}  // namespace

namespace fetch {
namespace ledger {

/**
 * Constructs a execution manager instance
 *
 * @param num_executors The specified number of executors (and threads)
 */
// std::make_shared<fetch::network::ThreadPool>(num_executors)
ExecutionManager::ExecutionManager(std::string const &storage_path, std::size_t num_executors,
                                   StorageUnitPtr storage, ExecutorFactory const &factory)
  : storage_(std::move(storage))
  , idle_executors_(num_executors)
  , thread_pool_(network::MakeThreadPool(num_executors, "ExecutionManager"))
{
  // define all the file paths for the databases
  FilePaths const state_archive_paths = FilePaths::Create(storage_path, "exec_state_bookmark_map");
  FilePaths const block_state_paths   = FilePaths::Create(storage_path, "exec_block_state_map");

  // load state archive updates
  {
    std::lock_guard<Mutex> lock(state_archive_lock_);
    state_archive_.New(state_archive_paths.data_path, state_archive_paths.index_path);
    block_state_cache_.New(block_state_paths.data_path, block_state_paths.index_path);
  }

  // setup the executor pool
  {
    std::lock_guard<Mutex> lock(idle_executors_lock_);

    // ensure lists are reserved
    idle_executors_.reserve(num_executors);

    // create the executor instances
    for (std::size_t i = 0; i < num_executors; ++i)
    {
      idle_executors_.emplace_back(factory());
    }
  }
}

/**
 * Initiates the execution of a given block across the set of executors
 *
 * @param block The block to be executed
 * @return the status of the execution
 */
ExecutionManager::Status ExecutionManager::Execute(Block const &block)
{

  // if the execution manager is not running then no further transactions
  // should be scheduled
  if (!running_)
  {
    return Status::NOT_STARTED;
  }

  // if a block is currently being processed we should not allow another block
  // to be processed
  if (IsActive())
  {
    return Status::ALREADY_RUNNING;
  }

  // if we have seen the transactions for this block simply revert to the known
  // state of the block
  if (AttemptRestoreToBlock(block.hash))
  {
    last_block_hash_ = block.hash;
    return Status::COMPLETE;
  }

  // determine if the current block doesn't follow on from the previous block
  if (block.previous_hash != last_block_hash_)
  {
    if (!AttemptRestoreToBlock(block.previous_hash))
    {
      return Status::NO_PARENT_BLOCK;
    }
  }

  // TODO(issue 33): Detect and handle number of lanes updates

  // plan the execution for this block
  if (!PlanExecution(block))
  {
    return Status::UNABLE_TO_PLAN;
  }

  // update the last block hash
  last_block_hash_ = block.hash;
  num_slices_      = block.slices.size();

  // trigger the monitor / dispatch thread
  {
    std::lock_guard<std::mutex> lock(monitor_lock_);
    monitor_wake_.notify_one();
  }

  return Status::SCHEDULED;
}

/**
 * Given a input block, plan the execution of the transactions across the lanes
 * and slices
 *
 * @param block The input block to plan
 * @return true if successful, otherwise false
 */
bool ExecutionManager::PlanExecution(Block const &block)
{
  std::lock_guard<Mutex> lock(execution_plan_lock_);

  // clear and resize the execution plan
  execution_plan_.clear();
  execution_plan_.resize(block.slices.size());

  //  FETCH_LOG_INFO(LOGGING_NAME,"Planning ", block.slices.size(), " slices...");

  std::size_t slice_index = 0;
  for (auto const &slice : block.slices)
  {
    auto &slice_plan = execution_plan_[slice_index];

    //    FETCH_LOG_INFO(LOGGING_NAME,"Planning slice ", slice_index, "...");

    // process the transactions
    for (auto const &tx : slice.transactions)
    {
      Identifier id;
      id.Parse(tx.contract_name);
      auto contract = contracts_.Lookup(id.name_space());

      if (contract)
      {
        auto item = std::make_unique<ExecutionItem>(tx.transaction_hash, slice_index);

        // transform the resources into lane allocation
        for (auto const &resource : tx.resources)
        {
          storage::ResourceID const id{contract->CreateStateIndex(resource)};
          item->AddLane(id.lane(block.log2_num_lanes));
        }

        // insert the item into the execution plan
        slice_plan.emplace_back(std::move(item));
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to plan execution of tx: ",
                       byte_array::ToBase64(tx.transaction_hash));
        return false;
      }
    }

    ++slice_index;
  }

  return true;
}

/**
 * Dispatches an execution item to the next available executor
 *
 * This function should be called from a context of a thread pool
 *
 * @param item The execution item to dispatch
 */
void ExecutionManager::DispatchExecution(ExecutionItem &item)
{
  ExecutorPtr executor;

  // lookup a free executor
  {
    std::lock_guard<Mutex> lock(idle_executors_lock_);
    if (!idle_executors_.empty())
    {
      executor = idle_executors_.back();
      idle_executors_.pop_back();
    }
  }

  // We must have a executor present for this to work. This should always
  // be the case provided num_executors == num_threads (in thread pool)
  assert(executor);

  if (executor)
  {

    ++active_count_;
    auto status = item.Execute(*executor);
    (void)status;
    // TODO(issue 33):  Should check the status of the execution

    --active_count_;
    --remaining_executions_;
    ++completed_executions_;

    {
      std::lock_guard<Mutex> lock(idle_executors_lock_);
      idle_executors_.push_back(executor);
    }

    // trigger the monitor thread to process the next slice if needed
    monitor_notify_.notify_one();
  }
}

/**
 * Starts the execution manager running
 */
void ExecutionManager::Start()
{
  running_ = true;

  // create and start the monitor thread
  monitor_thread_ = std::make_unique<std::thread>(&ExecutionManager::MonitorThreadEntrypoint, this);

  // wait for the monitor thread to be setup
  for (std::size_t i = 0; i < MAX_STARTUP_ITERATIONS; ++i)
  {
    if (monitor_ready_)
    {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{STARTUP_ITERATION_TIME_MS});
  }

  if (!monitor_ready_)
  {
    throw std::runtime_error("Failed waiting for the monitor to start");
  }

  // fire up the main worker thread pool
  thread_pool_->Start();
}

/**
 * Stops the execution manager running
 */
void ExecutionManager::Stop()
{
  running_ = false;

  // trigger the monitor thread to wake up (so that it sees the change in
  // running_)
  {
    std::lock_guard<std::mutex> lock(monitor_lock_);
    monitor_wake_.notify_all();
    monitor_notify_.notify_all();
  }

  // wait for the monitor thread to exit
  monitor_thread_->join();
  monitor_thread_.reset();

  // tear down the thread pool
  thread_pool_->Stop();
}

ExecutionManagerInterface::BlockHash ExecutionManager::LastProcessedBlock()
{
  // TODO(issue 33): thread safety
  return last_block_hash_;
}

bool ExecutionManager::IsActive()
{
  return active_;
}

bool ExecutionManager::IsIdle()
{
  return !active_;
}

bool ExecutionManager::Abort()
{
  return false;
}

void ExecutionManager::MonitorThreadEntrypoint()
{
  enum class MonitorState
  {
    IDLE,
    SCHEDULE_NEXT_SLICE,
    RUNNING,
    BOOKMARKING_STATE
  };

  MonitorState state = MonitorState::IDLE;

  std::size_t next_slice = 0;

  BlockHash current_block;

  while (running_)
  {
    monitor_ready_ = true;

    switch (state)
    {
    case MonitorState::IDLE:
    {
      active_ = false;

      // enter the idle state where we wait for the next block to be posted
      {
        std::unique_lock<std::mutex> lock(monitor_lock_);
        monitor_wake_.wait(lock);
      }

      active_       = true;
      current_block = last_block_hash_;

      // schedule the next slice if we have been triggered
      if (running_)
      {
        state      = MonitorState::SCHEDULE_NEXT_SLICE;
        next_slice = 0;
      }

      break;
    }

    case MonitorState::SCHEDULE_NEXT_SLICE:
    {
      std::lock_guard<Mutex> lock(execution_plan_lock_);

      if (execution_plan_.empty())
      {
        state = MonitorState::BOOKMARKING_STATE;
      }
      else
      {

        auto const &slice_plan = execution_plan_[next_slice];

        // determine the target number of executions being expected (must be
        // done before the thread pool dispatch)
        remaining_executions_ = slice_plan.size();

        for (auto &item : slice_plan)
        {
          // create the closure and dispatch to the thread pool
          auto self = shared_from_this();
          thread_pool_->Post([self, &item]() { self->DispatchExecution(*item); });
        }

        state = MonitorState::RUNNING;
        ++next_slice;
      }

      break;
    }

    case MonitorState::RUNNING:
    {

      // wait for the execution to complete
      if (remaining_executions_ > 0)
      {
        std::unique_lock<std::mutex> lock(monitor_lock_);
        monitor_notify_.wait_for(lock, std::chrono::milliseconds{100});
      }

      // determine the next state (provided we have complete
      if (remaining_executions_ == 0)
      {
        state = (num_slices_ > next_slice) ? MonitorState::SCHEDULE_NEXT_SLICE
                                           : MonitorState::BOOKMARKING_STATE;
      }

      break;
    }

    case MonitorState::BOOKMARKING_STATE:
    {
      // calculate the final state hash
      crypto::SHA256 hash;
      hash.Update(storage_->Hash());
      StateHash state_hash = hash.Final();

      // request a bookmark
      Bookmark bookmark     = 0;
      bool     new_bookmark = false;

      if (state_hash.size())
      {
        std::lock_guard<Mutex> lock(state_archive_lock_);
        new_bookmark = state_archive_.AllocateBookmark(state_hash, bookmark);
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unable to request state hash");
      }

      // only need to commit the new bookmark if there is actually a change in
      // state
      if (new_bookmark)
      {
        // commit the changes in state
        try
        {
          storage_->Commit(bookmark);
        }
        catch (std::exception &ex)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Unable to commit state. Error: ", ex.what());
        }

        // update the state archives
        {
          std::lock_guard<Mutex> lock(state_archive_lock_);
          if (!state_archive_.ConfirmBookmark(state_hash, bookmark))
          {
            FETCH_LOG_WARN(LOGGING_NAME, "Unable to confirm bookmark: ", bookmark);
          }

          // update the block state cache
          block_state_cache_.Set(ResourceID{current_block}, state_hash);
        }
      }

      // finished processing the block
      state = MonitorState::IDLE;

      break;
    }
    }
  }
}

bool ExecutionManager::AttemptRestoreToBlock(BlockHash const &digest)
{
  bool success = false;

  assert(digest.size() == 32);

  std::lock_guard<Mutex> lock(state_archive_lock_);

  // need to load the state from a previous application, so lookup the
  // corresponding state hash
  StateHash state_hash{};
  if (block_state_cache_.Get(ResourceID{digest}, state_hash))
  {
    Bookmark bookmark{0};

    // lookup the bookmark
    if (state_archive_.LookupBookmark(state_hash, bookmark))
    {
      // revert
      storage_->Revert(bookmark);
      success = true;
    }
  }

  return success;
}

}  // namespace ledger
}  // namespace fetch
