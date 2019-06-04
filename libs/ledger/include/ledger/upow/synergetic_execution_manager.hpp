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

#include "ledger/chain/digest.hpp"
#include "ledger/chain/address.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_package.hpp"
#include "ledger/upow/synergetic_execution_manager_interface.hpp"

#include "ledger/dag/dag.hpp"

#include <unordered_map>

namespace fetch {
namespace ledger {

class StorageUnitInterface;

class SynergeticExecutionManager : public SynergeticExecutionManagerInterface
{
public:

  using DAGPtr                 = std::shared_ptr<ledger::DAG>;

  // Construction / Destruction
  SynergeticExecutionManager(DAGPtr, StorageUnitInterface &);
  SynergeticExecutionManager(SynergeticExecutionManager const &) = delete;
  SynergeticExecutionManager(SynergeticExecutionManager &&) = delete;
  ~SynergeticExecutionManager() override = default;

  /// @name Synergetic Execution Manager Interface
  /// @{
  ExecStatus PrepareWorkQueue(Block const &current, Block const &previous) override;
  bool ValidateWorkAndUpdateState() override;
  /// @}

  // Operators
  SynergeticExecutionManager &operator=(SynergeticExecutionManager const &) = delete;
  SynergeticExecutionManager &operator=(SynergeticExecutionManager &&) = delete;

private:
  using WorkMap = std::unordered_map<Digest, WorkPackagePtr, DigestHashAdapter>;

  DAGPtr dag_;
  StorageUnitInterface &storage_;

  WorkMap solution_queues_;
};

} // namespace ledger
} // namespace fetch
