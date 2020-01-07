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

#include "core/assert.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/mutex.hpp"
#include "core/set_thread_name.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/consensus/stake_update_event.hpp"
#include "ledger/execution_manager.hpp"
#include "ledger/executor.hpp"
#include "ledger/state_adapter.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "logging/logging.hpp"
#include "moment/deadline_timer.hpp"
#include "storage/resource_mapper.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

static constexpr char const *LOGGING_NAME              = "ExecutionManager";
static constexpr std::size_t MAX_STARTUP_ITERATIONS    = 20;
static constexpr std::size_t STARTUP_ITERATION_TIME_MS = 100;

namespace fetch {
namespace ledger {

using telemetry::Registry;

/**
 * Constructs a execution manager instance
 *
 * @param num_executors The specified number of executors (and threads)
 */
ExecutionManager::ExecutionManager(std::size_t num_executors, uint32_t log2_num_lanes,
                                   StorageUnitPtr storage, ExecutorFactory const &factory,
                                   TransactionStatusCache::ShrdPtr tx_status_cache)
  : log2_num_lanes_{log2_num_lanes}
  , storage_{std::move(storage)}
  , thread_pool_{network::MakeThreadPool(num_executors, "Executor")}
  , tx_status_cache_{std::move(tx_status_cache)}
  , tx_executed_count_(Registry::Instance().CreateCounter(
        "ledger_exec_mgr_tx_executed_total", "The total number of executed transactions"))
  , slices_executed_count_(Registry::Instance().CreateCounter(
        "ledger_exec_mgr_slice_executed_total", "The total number of executed slices"))
  , fees_settled_count_(Registry::Instance().CreateCounter(
        "ledger_exec_mgr_fees_settled_total", "The total number of settle fees rounds"))
  , blocks_completed_count_(Registry::Instance().CreateCounter(
        "ledger_exec_mgr_blocks_completed_total", "The total number of settle fees rounds"))
  , execution_duration_(Registry::Instance().CreateHistogram(
        {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
         0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
         0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
         0.001,    0.01,     0.1,      1,        10.,      100.},
        "ledger_exec_mgr_block_duration", "The execution duration in seconds for blocks"))
{
  // create all the executor metrics
  Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_executor_overall_duration",
      "The execution duration in seconds for executing a transaction");
  Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_executor_tx_retrieve_duration",
      "The execution duration in seconds for retrieving the transaction");
  Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_executor_validation_checks_duration",
      "The execution duration in seconds for performming the pre-validation checks transaction");
  Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_executor_contract_execution_duration",
      "The execution duration in seconds for executing the chain code or smart contract");
  Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_executor_transfers_duration",
      "The execution duration in seconds for executing the transfers of a transaction");
  Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_executor_deduct_fees_duration",
      "The execution duration in seconds for executing a transaction");
  Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_executor_settle_fees_duration",
      "The execution duration in seconds for executing a transaction");

  // setup the executor pool
  {
    FETCH_LOCK(idle_executors_lock_);

    // ensure lists are reserved
    idle_executors_.reserve(num_executors);

    // create the executor instances
    for (std::size_t i = 0; i < num_executors; ++i)
    {
      auto executor = factory();
      assert(static_cast<bool>(executor));

      idle_executors_.emplace_back(std::move(executor));
    }
  }
}

/**
 * Initiates the execution of a given block across the set of executors
 *
 * @param block The block to be executed
 * @return the status of the execution
 */
ExecutionManager::ScheduleStatus ExecutionManager::Execute(Block const &block)
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
  state_.ApplyVoid([&block](Summary &summary) {
    summary.last_block_hash   = block.hash;
    summary.last_block_miner  = chain::Address(block.miner_id);
    summary.last_block_number = block.block_number;
    summary.state             = State::ACTIVE;
  });
  num_slices_ = block.slices.size();

  // trigger the monitor / dispatch thread
  {
    FETCH_LOCK(monitor_lock_);
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
bool ExecutionManager::PlanExecution(Block const &block)
{
  FETCH_LOCK(execution_plan_lock_);

  // clear and resize the execution plan
  execution_plan_.clear();
  execution_plan_.resize(block.slices.size());

  uint64_t slice_index = 0;
  for (auto const &slice : block.slices)
  {
    auto &slice_plan = execution_plan_[slice_index];

    // process the transactions
    for (auto const &tx : slice)
    {
      // ensure each of the layouts are correctly formatted. This should be removed in the future
      // and some level of dynamic scaling should be applied.
      assert((1u << log2_num_lanes_) == tx.mask().size());

      // insert the item into the execution plan
      slice_plan.emplace_back(
          std::make_unique<ExecutionItem>(tx.digest(), block.block_number, slice_index, tx.mask()));
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

  // look up a free executor
  {
    FETCH_LOCK(idle_executors_lock_);
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
    // increment the active counters
    counters_.ApplyVoid([](auto &counters) { ++counters.active; });

    // execute the item
    item.Execute(*executor);
    auto const &result{item.result()};

    // determine what the status is
    if (ExecutorInterface::Status::SUCCESS != result.status)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Error executing tx: 0x", item.digest().ToHex(),
                     " status: ", ledger::ToString(result.status));
    }

    counters_.ApplyVoid([](auto &counters) {
      --counters.active;
      --counters.remaining;
    });

    ++completed_executions_;
    tx_executed_count_->increment();

    {
      FETCH_LOCK(idle_executors_lock_);
      idle_executors_.push_back(std::move(executor));
    }
  }
  else
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Failed to secure an idle executor");
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
    FETCH_LOCK(monitor_lock_);
    monitor_wake_.notify_all();
    monitor_notify_.notify_all();
  }

  // wait for the monitor thread to exit
  if (monitor_thread_)
  {
    monitor_thread_->join();
    monitor_thread_.reset();
  }

  // tear down the thread pool
  thread_pool_->Stop();
}

void ExecutionManager::SetLastProcessedBlock(Digest hash)
{
  state_.ApplyVoid([&hash](Summary &summary) { summary.last_block_hash = std::move(hash); });
}

Digest ExecutionManager::LastProcessedBlock() const
{
  return state_.Apply([](Summary const &summary) { return summary.last_block_hash; });
}

ExecutionManager::State ExecutionManager::GetState()
{
  return state_.Apply([](Summary const &summary) { return summary.state; });
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
    SETTLE_FEES,
    BOOKMARKING_STATE
  };

  MonitorState monitor_state = MonitorState::COMPLETED;

  std::size_t       current_slice        = 0;
  uint64_t          aggregate_block_fees = 0;
  StakeUpdateEvents aggregated_stake_events{};

  Digest current_block;

  while (running_)
  {
    monitor_ready_ = true;

    switch (monitor_state)
    {
    case MonitorState::FAILED:
      FETCH_LOG_WARN(LOGGING_NAME, "Execution Engine experience fatal error");

      state_.ApplyVoid([](Summary &summary) { summary.state = State::EXECUTION_FAILED; });
      monitor_state = MonitorState::IDLE;
      break;

    case MonitorState::STALLED:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Now Stalled");

      state_.ApplyVoid([](Summary &summary) { summary.state = State::TRANSACTIONS_UNAVAILABLE; });
      monitor_state = MonitorState::IDLE;
      break;

    case MonitorState::COMPLETED:
      FETCH_LOG_DEBUG(LOGGING_NAME, "Now Complete");

      state_.ApplyVoid([](Summary &summary) { summary.state = State::IDLE; });
      monitor_state = MonitorState::IDLE;
      break;

    case MonitorState::IDLE:
    {
      blocks_completed_count_->increment();

      state_.ApplyVoid([](Summary &summary) { summary.state = State::IDLE; });

      FETCH_LOG_DEBUG(LOGGING_NAME, "Now Idle");

      // enter the idle state where we wait for the next block to be posted
      {
        std::unique_lock<std::mutex> lock(monitor_lock_);
        monitor_wake_.wait(lock);
      }

      current_block = state_.Apply([](Summary &summary) {
        summary.state = State::ACTIVE;
        return summary.last_block_hash;
      });

      FETCH_LOG_DEBUG(LOGGING_NAME, "Now Active");

      // schedule the next slice if we have been triggered
      if (running_)
      {
        monitor_state        = MonitorState::SCHEDULE_NEXT_SLICE;
        current_slice        = 0;
        aggregate_block_fees = 0;
        aggregated_stake_events.clear();
      }

      break;
    }

    case MonitorState::SCHEDULE_NEXT_SLICE:
    {
      FETCH_LOCK(execution_plan_lock_);

      slices_executed_count_->increment();

      if (execution_plan_.empty())
      {
        monitor_state = MonitorState::SETTLE_FEES;
      }
      else
      {
        auto const &slice_plan = execution_plan_[current_slice];

        // determine the target number of executions being expected (must be
        // done before the thread pool dispatch)
        counters_.ApplyVoid([&slice_plan](auto &counters) {
          counters = Counters{0, slice_plan.size()};
        });

        auto self = shared_from_this();
        for (auto &item : slice_plan)
        {
          // create the closure and dispatch to the thread pool
          thread_pool_->Post([self, &item]() {
            telemetry::FunctionTimer const timer{*(self->execution_duration_)};
            self->DispatchExecution(*item);
          });
        }

        monitor_state = MonitorState::RUNNING;
      }

      break;
    }

    case MonitorState::RUNNING:
    {
      // wait for the execution to complete
      bool const finished =
          counters_.Wait([](auto const &counters) -> bool { return counters.remaining == 0; },
                         std::chrono::seconds{2});

      if (!finished)
      {
        counters_.ApplyVoid([](auto const &counters) {
          FETCH_LOG_WARN(LOGGING_NAME, "### Extra long execution: remaining: ", counters.remaining);
        });
      }
      else
      {
        // evaluate the status of the executions
        std::size_t num_complete{0};
        std::size_t num_stalls{0};
        std::size_t num_errors{0};
        std::size_t num_fatal_errors{0};

        // look through all execution items and determine if it was successful
        for (auto const &item : execution_plan_[current_slice])
        {
          assert(item);

          switch (Categorise(item->result().status))
          {
          case ExecutionStatusCategory::SUCCESS:
            ++num_complete;
            break;

          case ExecutionStatusCategory::NORMAL_ERROR:
            ++num_errors;
            break;

          case ExecutionStatusCategory::INTERNAL_ERROR:
            ++num_stalls;
            break;

          case ExecutionStatusCategory::BLOCK_INVALIDATING_ERROR:
          default:
            ++num_fatal_errors;
            break;
          }

          // update aggregate fees
          aggregate_block_fees += item->fee();
          item->AggregateStakeUpdates(aggregated_stake_events);

          if (tx_status_cache_)
          {
            tx_status_cache_->Update(item->digest(), item->result());
          }
        }

        // only provide debug if required
        if ((num_complete + num_stalls + num_errors + num_fatal_errors) != 0u)
        {
          if ((num_stalls + num_errors + num_fatal_errors) != 0u)
          {
            FETCH_LOG_WARN(LOGGING_NAME, "Slice ", current_slice,
                           " Execution Status - Complete: ", num_complete, " Stalls: ", num_stalls,
                           " Errors: ", num_errors, " Fatal Errors: ", num_fatal_errors);
          }
          else
          {
            FETCH_LOG_DEBUG(LOGGING_NAME, "Slice ", current_slice,
                            " Execution Status - Complete: ", num_complete, " Stalls: ", num_stalls,
                            " Errors: ", num_errors);
          }
        }

        // increment the slice counter
        ++current_slice;

        // decide the next monitor state based on the status of the slice execution
        if (num_fatal_errors != 0u)
        {
          monitor_state = MonitorState::FAILED;
        }
        else if (num_stalls != 0u)
        {
          monitor_state = MonitorState::STALLED;
        }
        else if (num_slices_ > current_slice)
        {
          monitor_state = MonitorState::SCHEDULE_NEXT_SLICE;
        }
        else
        {
          monitor_state = MonitorState::SETTLE_FEES;
        }
      }
      break;
    }

    case MonitorState::SETTLE_FEES:
    {
      moment::DeadlineTimer executor_deadline("ExecMgr");
      executor_deadline.Restart(1000u);

      // In rare cases due to scheduling, no executors might have been returned to the idle queue.
      // This busy wait loop will catch this event and has a fixed duration.
      for (;;)
      {
        if (executor_deadline.HasExpired())
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Unable to locate free executor to settle miner fees");
          break;
        }

        // attempt to settle the fees using one of the free executors
        {
          FETCH_LOCK(idle_executors_lock_);
          if (!idle_executors_.empty())
          {
            // lookup the last block miner
            chain::Address last_block_miner;
            BlockIndex     last_block_number{0};

            // extract the information from the summary structure
            state_.ApplyVoid([&last_block_miner, &last_block_number](Summary const &summary) {
              last_block_miner  = summary.last_block_miner;
              last_block_number = summary.last_block_number;
            });

            // get the first one and settle the fees
            idle_executors_.front()->SettleFees(last_block_miner, last_block_number,
                                                aggregate_block_fees, log2_num_lanes_,
                                                aggregated_stake_events);
            fees_settled_count_->increment();
            break;
          }
        }

        // due to the race on the idle executors wait here
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
      }

      // move on to the next state
      monitor_state = MonitorState::BOOKMARKING_STATE;
      break;
    }

    case MonitorState::BOOKMARKING_STATE:
      // finished processing the block
      monitor_state = MonitorState::IDLE;
      break;
    }
  }
}

}  // namespace ledger
}  // namespace fetch
