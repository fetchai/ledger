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
#include "core/threading.hpp"
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
ExecutionManager::ExecutionManager(std::size_t num_executors, StorageUnitPtr storage,
                                   ExecutorFactory const &factory)
  : storage_(std::move(storage))
  , idle_executors_(num_executors)
  , thread_pool_(network::MakeThreadPool(num_executors, "Executor"))
{
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
ExecutionManager::ScheduleStatus ExecutionManager::Execute(Block::Body const &block)
{
  // if the execution manager is not running then no further transactions
  // should be scheduled
  if (!running_)
  {
    return ScheduleStatus::NOT_STARTED;
  }

  // cache the current state
  if (State::IDLE != GetState())
  {
    return ScheduleStatus::ALREADY_RUNNING;
  }

  // TODO(issue 33): Detect and handle number of lanes updates

  // plan the execution for this block
  if (!PlanExecution(block))
  {
    return ScheduleStatus::UNABLE_TO_PLAN;
  }

  // update the last block hash
  last_block_hash_ = block.hash;
  num_slices_      = block.slices.size();

  // update the state otherwise there is a race between when the executor thread wakes up
  SetState(State::ACTIVE);

  // trigger the monitor / dispatch thread
  {
    std::lock_guard<std::mutex> lock(monitor_lock_);
    monitor_wake_.notify_one();
  }

  return ScheduleStatus::SCHEDULED;
}

/**
 * Given a input block, plan the execution of the transactions across the lanes
 * and slices
 *
 * @param block The input block to plan
 * @return true if successful, otherwise false
 */
bool ExecutionManager::PlanExecution(Block::Body const &block)
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
    for (auto const &tx : slice)
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

    // execute the item
    item.Execute(*executor);

    --active_count_;
    --remaining_executions_;
    ++completed_executions_;

    {
      std::lock_guard<Mutex> lock(idle_executors_lock_);
      idle_executors_.push_back(executor);
    }

    // trigger the monitor thread to process the next slice if needed
    {
      std::lock_guard<Mutex> lock(monitor_lock_);
      monitor_notify_.notify_all();
    }
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

void ExecutionManager::SetLastProcessedBlock(BlockHash hash)
{
  // TODO(issue 33): thread safety
  last_block_hash_ = hash;
}

ExecutionManagerInterface::BlockHash ExecutionManager::LastProcessedBlock()
{
  // TODO(issue 33): thread safety
  return last_block_hash_;
}

ExecutionManager::State ExecutionManager::GetState()
{
  std::lock_guard<Mutex> lock(state_lock_);
  return state_;
}

bool ExecutionManager::Abort()
{
  // TODO(private issue 533): Implement user execution abort
  return false;
}

void ExecutionManager::MonitorThreadEntrypoint()
{
  SetThreadName("ExecMgrMon");

  enum class MonitorState
  {
    FAILED,
    STALLED,
    COMPLETED,
    IDLE,
    SCHEDULE_NEXT_SLICE,
    RUNNING,
    BOOKMARKING_STATE
  };

  MonitorState monitor_state = MonitorState::COMPLETED;

  std::size_t current_slice = 0;

  BlockHash current_block;

  while (running_)
  {
    monitor_ready_ = true;

    switch (monitor_state)
    {
    case MonitorState::FAILED:
      FETCH_LOG_INFO(LOGGING_NAME, "Now Failed");

      SetState(State::EXECUTION_FAILED);
      monitor_state = MonitorState::IDLE;
      break;

    case MonitorState::STALLED:
      FETCH_LOG_INFO(LOGGING_NAME, "Now Stalled");

      SetState(State::TRANSACTIONS_UNAVAILABLE);
      monitor_state = MonitorState::IDLE;
      break;

    case MonitorState::COMPLETED:
      FETCH_LOG_INFO(LOGGING_NAME, "Now Complete");

      SetState(State::IDLE);
      monitor_state = MonitorState::IDLE;
      break;

    case MonitorState::IDLE:
    {
      SetState(State::IDLE);

      FETCH_LOG_INFO(LOGGING_NAME, "Now Idle");

      // enter the idle state where we wait for the next block to be posted
      {
        std::unique_lock<std::mutex> lock(monitor_lock_);
        monitor_wake_.wait(lock);
      }

      SetState(State::ACTIVE);
      current_block = last_block_hash_;

      FETCH_LOG_INFO(LOGGING_NAME, "Now Active");

      // schedule the next slice if we have been triggered
      if (running_)
      {
        monitor_state = MonitorState::SCHEDULE_NEXT_SLICE;
        current_slice = 0;
      }

      break;
    }

    case MonitorState::SCHEDULE_NEXT_SLICE:
    {
      std::lock_guard<Mutex> lock(execution_plan_lock_);

      if (execution_plan_.empty())
      {
        monitor_state = MonitorState::BOOKMARKING_STATE;
      }
      else
      {
        auto const &slice_plan = execution_plan_[current_slice];

        // determine the target number of executions being expected (must be
        // done before the thread pool dispatch)
        remaining_executions_ = slice_plan.size();

        for (auto &item : slice_plan)
        {
          // create the closure and dispatch to the thread pool
          auto self = shared_from_this();
          thread_pool_->Post([self, &item]() { self->DispatchExecution(*item); });
        }

        monitor_state = MonitorState::RUNNING;
      }

      break;
    }

    case MonitorState::RUNNING:
    {
      if (remaining_executions_ > 0xFFFFFF)
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Potential wrap around of counter");
      }

      // wait for the execution to complete
      if (remaining_executions_ > 0)
      {
        std::unique_lock<std::mutex> lock(monitor_lock_);

        if (std::cv_status::timeout == monitor_notify_.wait_for(lock, std::chrono::milliseconds{2000}))
        {
          FETCH_LOG_WARN(LOGGING_NAME, "### Extra long execution: remaining: ", remaining_executions_.load());
        }
      }

      // determine the next state (provided we have complete
      if (remaining_executions_ == 0)
      {
        // evaluate the status of the executions
        std::size_t num_complete{0};
        std::size_t num_stalls{0};
        std::size_t num_errors{0};

        // look through all execution items and determine if it was successful
        for (auto const &item : execution_plan_[current_slice])
        {
          assert(item);

          switch (item->status())
          {
          case ExecutionItem::Status::SUCCESS:
            ++num_complete;
            break;
          case ExecutionItem::Status::TX_LOOKUP_FAILURE:
            ++num_stalls;
            break;
          case ExecutionItem::Status::NOT_RUN:
          case ExecutionItem::Status::RESOURCE_FAILURE:
          case ExecutionItem::Status::INEXPLICABLE_FAILURE:
          case ExecutionItem::Status::CHAIN_CODE_LOOKUP_FAILURE:
          case ExecutionItem::Status::CHAIN_CODE_EXEC_FAILURE:
            ++num_errors;
            break;
          }
        }

        // only provide debug if required
        if (num_complete + num_stalls + num_errors)
        {
          if (num_stalls + num_errors)
          {
            FETCH_LOG_WARN(LOGGING_NAME, "Slice ", current_slice,
                           " Execution Status - Complete: ", num_complete, " Stalls: ", num_stalls,
                           " Errors: ", num_errors);
          }
          else
          {
            FETCH_LOG_INFO(LOGGING_NAME, "Slice ", current_slice,
                          " Execution Status - Complete: ", num_complete, " Stalls: ", num_stalls,
                          " Errors: ", num_errors);

          }
        }

        // increment the slice counter
        ++current_slice;

        // decide the next monitor state based on the status of the slice execution
        if (num_errors)
        {
          monitor_state = MonitorState::FAILED;
        }
        else if (num_stalls)
        {
          monitor_state = MonitorState::STALLED;
        }
        else if (num_slices_ > current_slice)
        {
          monitor_state = MonitorState::SCHEDULE_NEXT_SLICE;
        }
        else
        {
          monitor_state = MonitorState::BOOKMARKING_STATE;
        }
      }

      break;
    }

    case MonitorState::BOOKMARKING_STATE:

      // trigger the storage to archive the current state
      storage_->Commit();

      // finished processing the block
      monitor_state = MonitorState::IDLE;
      break;
    }
  }
}

}  // namespace ledger
}  // namespace fetch
