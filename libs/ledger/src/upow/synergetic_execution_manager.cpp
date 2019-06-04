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
#include "ledger/upow/synergetic_execution_manager.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"

namespace fetch {
namespace ledger {
namespace {

//constexpr char const *LOGGING_NAME = "SynExecMgr";

using ExecStatus = SynergeticExecutionManager::ExecStatus;

} // namespace

SynergeticExecutionManager::SynergeticExecutionManager(DAGPtr dag, StorageUnitInterface &storage)
  : dag_{dag}
  , storage_{storage}
{
  FETCH_UNUSED(storage_);
}

ExecStatus SynergeticExecutionManager::PrepareWorkQueue(Block const &current, Block const &previous)
{
  FETCH_UNUSED(current);
  FETCH_UNUSED(previous);

//  solution_queues_.clear();
//  WorkMap synergetic_work_candidates;
//
//  // Annotating nodes in the DAG
//  if (!dag_.SetNodeTime(current))
//  {
//    return ExecStatus::INVALID_BLOCK;
//  }
//
//  // Work is certified by current block
//  auto segment = dag_.ExtractSegment(current);
//
//  // but data is used from the previous block
////  miner_.SetBlock(previous);
////  assert((previous.body.block_number + 1) == current.body.block_number);
//
//  std::size_t index{0};
//  for (auto const &item : segment)
//  {
//    FETCH_LOG_INFO(LOGGING_NAME, "Work ", index, " Hash: 0x", item.hash.ToHex(), " Type: ", item.type);
//
//    ++index;
//  }

  return SUCCESS;
}

bool SynergeticExecutionManager::ValidateWorkAndUpdateState()
{
  return true;
}

} // namespace ledger
} // namespace fetch
