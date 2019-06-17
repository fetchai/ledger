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
#include "ledger/chain/address.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/upow/synergetic_execution_manager_interface.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_package.hpp"
#include "ledger/upow/work_queue.hpp"
#include "vectorise/threading/pool.hpp"

#include "ledger/dag/dag_interface.hpp"

#include <functional>
#include <unordered_map>

namespace fetch {
namespace ledger {

class StorageUnitInterface;
class SynergeticExecutorInterface;

class SynergeticExecutionManager : public SynergeticExecutionManagerInterface
{
public:
  using ExecutorPtr     = std::shared_ptr<SynergeticExecutorInterface>;
  using ExecutorFactory = std::function<ExecutorPtr()>;
  using DAGPtr          = std::shared_ptr<DAGInterface>;
  using DAGNodePtr      = std::shared_ptr<DAGNode>;
  using ConstByteArray  = byte_array::ConstByteArray;
  using ProblemData     = std::vector<ConstByteArray>;

  // Construction / Destruction
  SynergeticExecutionManager(DAGPtr dag, std::size_t num_executors, ExecutorFactory const &);
  SynergeticExecutionManager(SynergeticExecutionManager const &) = delete;
  SynergeticExecutionManager(SynergeticExecutionManager &&)      = delete;
  ~SynergeticExecutionManager() override                         = default;

  /// @name Synergetic Execution Manager Interface
  /// @{
  ExecStatus PrepareWorkQueue(Block const &current, Block const &previous) override;
  bool       ValidateWorkAndUpdateState(uint64_t block, std::size_t num_lanes) override;
  /// @}

  // Operators
  SynergeticExecutionManager &operator=(SynergeticExecutionManager const &) = delete;
  SynergeticExecutionManager &operator=(SynergeticExecutionManager &&) = delete;

private:
  struct WorkItem
  {
    WorkQueue   work_queue;
    ProblemData problem_data;
  };

  using Executors      = std::vector<ExecutorPtr>;
  using WorkItemPtr    = std::shared_ptr<WorkItem>;
  using WorkQueueStack = std::vector<WorkItemPtr>;
  using ThreadPool     = threading::Pool;
  using Mutex          = mutex::Mutex;

  void ExecuteItem(WorkQueue &queue, ProblemData const &problem_data, uint64_t block,
                   std::size_t num_lanes);

  // System Components
  DAGPtr dag_;

  /// @name Protected State
  /// @{
  Mutex          lock_{__LINE__, __FILE__};
  WorkQueueStack solution_stack_;
  Executors      executors_;
  ThreadPool     threads_;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
