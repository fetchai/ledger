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

#include "ledger/chain/constants.hpp"
#include "ledger/execution_manager_interface.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

class FakeStorageUnit;

class FakeExecutionManager : public fetch::ledger::ExecutionManagerInterface
{
public:
  using Block = fetch::ledger::Block;

  static constexpr char const *LOGGING_NAME = "FakeExexMgr";

  explicit FakeExecutionManager(FakeStorageUnit &storage);
  ~FakeExecutionManager() override = default;

  /// @name Execution Manager Interface
  /// @{
  ScheduleStatus Execute(Block::Body const &block) override;
  BlockHash      LastProcessedBlock() override;
  void           SetLastProcessedBlock(BlockHash hash) override;
  State          GetState() override;
  bool           Abort() override;
  /// @}

private:
  FakeStorageUnit &storage_;

  BlockHash   current_hash_;
  BlockHash   current_merkle_root_;
  BlockHash   last_processed_{fetch::ledger::GENESIS_DIGEST};
  std::size_t current_polls_{0};
};
