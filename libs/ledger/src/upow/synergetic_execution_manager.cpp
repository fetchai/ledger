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

#include "ledger/chain/block.hpp"
#include "ledger/upow/synergetic_execution_manager.hpp"
#include "ledger/upow/synergetic_executor_interface.hpp"
#include "logging/logging.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

namespace fetch {
namespace ledger {

constexpr char const *LOGGING_NAME = "SynExecMgr";

using ExecStatus = SynergeticExecutionManager::ExecStatus;

SynergeticExecutionManager::SynergeticExecutionManager(DAGPtr dag, std::size_t num_executors,
                                                       ExecutorFactory const &factory)
  : dag_{std::move(dag)}
  , executors_(num_executors)
  , threads_{num_executors, "SynEx"}
  , no_executor_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_upow_exec_manager_rid_no_executor_total",
        "The number of cases where ExecuteItem had missing executor.")}
  , no_executor_loop_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_upow_exec_manager_rid_no_executor_loop_iter_total",
        "The total number of iterations we had to make when executor was missing in ExecuteItem")}
  , execute_item_failed_count_{telemetry::Registry::Instance().CreateCounter(
        "ledger_upow_exec_manager_rid_no_executor_loop_fails_total",
        "Counts how many times ExecuteItem failed, because executor not available after wait.")}
{
  if (num_executors != 1)
  {
    throw std::runtime_error(
        "The number of executors must be 1 because state concurrency not implemented");
  }

  telemetry::Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_synergetic_executor_deduct_fees_duration",
      "The execution duration in seconds for executing a transaction");

  prepare_queue_duration_ = telemetry::Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_synergetic_executor_prepare_queue_duration",
      "Preparing work queue duration in seconds");

  execute_duration_ = telemetry::Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_synergetic_executor_execute_duration", "The execution duration in seconds");

  telemetry::Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_synergetic_executor_work_duration",
      "The execution duration in seconds for executing the work method of the contract");

  telemetry::Registry::Instance().CreateHistogram(
      {0.000001, 0.000002, 0.000003, 0.000004, 0.000005, 0.000006, 0.000007, 0.000008, 0.000009,
       0.00001,  0.00002,  0.00003,  0.00004,  0.00005,  0.00006,  0.00007,  0.00008,  0.00009,
       0.0001,   0.0002,   0.0003,   0.0004,   0.0005,   0.0006,   0.0007,   0.0008,   0.0009,
       0.001,    0.01,     0.1,      1,        10.,      100.},
      "ledger_synergetic_executor_complete_duration",
      "The execution duration in seconds for executing the complete method of the contract");

  // build the required number of executors
  for (auto &executor : executors_)
  {
    executor = factory();
  }
}

ExecStatus SynergeticExecutionManager::PrepareWorkQueue(Block const &current, Block const &previous)
{
  telemetry::FunctionTimer const timer{*prepare_queue_duration_};

  using WorkMap = std::unordered_map<chain::Address, WorkItemPtr>;

  auto const &current_epoch  = current.dag_epoch;
  auto const &previous_epoch = previous.dag_epoch;

  FETCH_LOG_DEBUG(LOGGING_NAME, "Preparing work queue for epoch: ", current_epoch.block_number);

  // Step 1. loop through all the solutions which were presented in this epoch
  WorkMap work_map{};
  for (auto const &digest : current_epoch.solution_nodes)
  {
    // look up the work from the block
    auto work = std::make_shared<Work>(current.block_number);
    if (!dag_->GetWork(digest, *work))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to get work from DAG Node: 0x", digest.ToHex());
      continue;
    }

    // look up (or create) the solution queue
    auto &work_item = work_map[work->address()];

    if (!work_item)
    {
      work_item = std::make_shared<WorkItem>();
    }

    // add the work to the queue
    work_item->work_queue.push(std::move(work));
  }

  // Step 2. Loop through previous epochs data in order to form the problem data
  DAGNode node{};
  for (auto const &digest : previous_epoch.data_nodes)
  {
    // look up the referenced DAG node
    if (!dag_->GetDAGNode(digest, node))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to retrieve referenced DAG node: 0x", digest.ToHex());
      continue;
    }

    // ensure the node is of data type
    if (node.type != DAGNode::DATA)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Invalid data node referenced in epoch: 0x", digest.ToHex());
      continue;
    }

    // attempt to look up the contract being referenced
    auto it = work_map.find(node.contract_address);
    if (it == work_map.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to look up references contract: address ",
                     node.contract_address.display());
      continue;
    }

    // add the problem into the work node
    it->second->problem_data.emplace_back(node.contents);
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Preparing work queue for epoch: ", current_epoch.block_number,
                  " (complete)");

  // Step 3. Update the final queue
  {
    FETCH_LOCK(lock_);

    solution_stack_.clear();
    solution_stack_.reserve(work_map.size());
    for (auto &item : work_map)
    {
      solution_stack_.emplace_back(std::move(item.second));
    }
    current_miner_ = chain::Address(current.miner_id);
  }

  return SUCCESS;
}

bool SynergeticExecutionManager::ValidateWorkAndUpdateState(std::size_t num_lanes)
{
  // get the current solution stack
  WorkQueueStack solution_stack;
  chain::Address miner;
  {
    FETCH_LOCK(lock_);
    std::swap(solution_stack, solution_stack_);
    miner = std::move(current_miner_);
  }

  // post all the work into the thread queues
  while (!solution_stack.empty())
  {
    // extract the solution queue
    auto work_item = solution_stack.back();
    solution_stack.pop_back();

    // dispatch the work
    threads_.Dispatch([this, work_item, num_lanes, miner] {
      ExecuteItem(work_item->work_queue, work_item->problem_data, num_lanes, miner);
    });
  }

  // wait for the execution to complete
  threads_.Wait();

  return true;
}

void SynergeticExecutionManager::ExecuteItem(WorkQueue &queue, ProblemData const &problem_data,
                                             std::size_t num_lanes, chain::Address const &miner)
{
  telemetry::FunctionTimer const timer{*execute_duration_};

  ExecutorPtr executor;

  bool first = true;
  for (uint8_t i = 0; i < 5; ++i)
  {
    {
      FETCH_LOCK(lock_);
      if (!executors_.empty())
      {
        break;
      }
    }
    if (first)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Executors empty, can't execute item! Waiting...");
      no_executor_count_->increment();
      first = false;
    }
    no_executor_loop_count_->increment();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

  // pick up an executor from the stack
  {
    FETCH_LOCK(lock_);
    if (executors_.empty())
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "ExecuteItem: executors empty after 500ms wait!");
      execute_item_failed_count_->increment();
      return;
    }
    executor = executors_.back();
    executors_.pop_back();
  }

  assert(static_cast<bool>(executor));
  executor->Verify(queue, problem_data, num_lanes, miner);

  // return the executor to the stack
  FETCH_LOCK(lock_);
  executors_.emplace_back(std::move(executor));
}

}  // namespace ledger
}  // namespace fetch
