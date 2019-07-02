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

#include "core/macros.hpp"
#include "ledger/chain/block.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/upow/synergetic_execution_manager.hpp"
#include "ledger/upow/synergetic_executor_interface.hpp"

namespace fetch {
namespace ledger {

constexpr char const *LOGGING_NAME = "SynExecMgr";

using ExecStatus = SynergeticExecutionManager::ExecStatus;

SynergeticExecutionManager::SynergeticExecutionManager(DAGPtr dag, std::size_t num_executors,
                                                       ExecutorFactory const &factory)
  : dag_{std::move(dag)}
  , executors_(num_executors)
  , threads_{num_executors, "SynEx"}
{
  if (num_executors != 1)
  {
    throw std::runtime_error(
        "The number of executors must be 1 because state concurrency not implemented");
  }

  // build the required number of executors
  for (auto &executor : executors_)
  {
    executor = factory();
  }
}

ExecStatus SynergeticExecutionManager::PrepareWorkQueue(Block const &current, Block const &previous)
{
  using WorkMap = std::unordered_map<Digest, WorkItemPtr, DigestHashAdapter>;

  auto const &current_epoch  = current.body.dag_epoch;
  auto const &previous_epoch = previous.body.dag_epoch;

  FETCH_LOG_INFO(LOGGING_NAME, "Preparing work queue for epoch: ", current_epoch.block_number);

  // Step 1. loop through all the solutions which were presented in this epoch
  WorkMap work_map{};
  for (auto const &digest : current_epoch.solution_nodes)
  {
    // lookup the work from the block
    auto work = std::make_shared<Work>();
    if (!dag_->GetWork(digest, *work))
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to get work from DAG. Node: 0x", digest.ToHex());
      continue;
    }

    // lookup (or create) the solution queue
    auto &work_item = work_map[work->contract_digest()];
    if (!work_item)
    {
      work_item = std::make_shared<WorkItem>();
    }

    // add the work to the queue
    work_item->work_queue.push(std::move(work));
  }

  // Step 2. Loop through previous epochs data in order form the problem data
  DAGNode node{};
  for (auto const &digest : previous_epoch.data_nodes)
  {
    // lookup the referenced DAG node
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

    // attempt to lookup the contract being referenced
    auto it = work_map.find(node.contract_digest);
    if (it == work_map.end())
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup references contract: 0x",
                     node.contract_digest.ToHex());
      continue;
    }

    // add the problem into the work node
    it->second->problem_data.emplace_back(node.contents);
  }

  FETCH_LOG_INFO(LOGGING_NAME, "Preparing work queue for epoch: ", current_epoch.block_number,
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
  }

  return SUCCESS;
}

bool SynergeticExecutionManager::ValidateWorkAndUpdateState(uint64_t block, std::size_t num_lanes)
{
  // get the current solution stack
  WorkQueueStack solution_stack;
  {
    FETCH_LOCK(lock_);
    std::swap(solution_stack, solution_stack_);
  }

  // post all the work into the thread queues
  while (!solution_stack.empty())
  {
    // extract the solution queue
    auto work_item = solution_stack.back();
    solution_stack.pop_back();

    // dispatch the work
    threads_.Dispatch([this, work_item, block, num_lanes] {
      ExecuteItem(work_item->work_queue, work_item->problem_data, block, num_lanes);
    });
  }

  // wait for the execution to complete
  threads_.Wait();

  return true;
}

void SynergeticExecutionManager::ExecuteItem(WorkQueue &queue, ProblemData const &problem_data,
                                             uint64_t block, std::size_t num_lanes)
{
  ExecutorPtr executor;

  // pick up an executor from the stack
  {
    FETCH_LOCK(lock_);
    executor = executors_.back();
    executors_.pop_back();
  }

  assert(static_cast<bool>(executor));
  executor->Verify(queue, problem_data, block, num_lanes);

  // return the executor to the stack
  FETCH_LOCK(lock_);
  executors_.emplace_back(std::move(executor));
}

}  // namespace ledger
}  // namespace fetch
