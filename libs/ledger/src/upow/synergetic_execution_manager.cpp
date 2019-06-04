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
#include "ledger/dag/dag_interface.hpp"
#include "ledger/upow/synergetic_execution_manager.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"

namespace fetch {
namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "SynExecMgr";

using ExecStatus = SynergeticExecutionManager::ExecStatus;

} // namespace

SynergeticExecutionManager::SynergeticExecutionManager(DAGInterface &d, StorageUnitInterface &s)
  : dag_{d}
  , storage_{s}
{
}

ExecStatus SynergeticExecutionManager::PrepareWorkQueue(Block const &current, Block const &previous)
{
  solution_queues_.clear();
  WorkMap synergetic_work_candidates;

  // Annotating nodes in the DAG
  if (!dag_.SetNodeTime(current))
  {
    return ExecStatus::INVALID_BLOCK;
  }

  // Work is certified by current block
  auto segment = dag_.ExtractSegment(current);

  // but data is used from the previous block
//  miner_.SetBlock(previous);
//  assert((previous.body.block_number + 1) == current.body.block_number);

  std::size_t index{0};
  for (auto const &item : segment)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Work ", index, " Hash: 0x", item.hash.ToHex(), " Type: ", item.type);

    ++index;
  }

//  // Extracting relevant contract names
//  for (auto &node : segment)
//  {
//    if (DAGNode::WORK == node.type)
//    {
//      // Deserialising work and adding it to the work map
//      Work work;
//      try
//      {
//        // extract the contents from the node
//        node.GetObject(work);
//
//        work.contract_name = node.contract_digest;
//        work.miner         = node.identity.identifier();
//        assert((current.body.block_number - 1) == work.block_number);
//      }
//      catch (std::exception const &e)
//      {
//        FETCH_LOG_WARN(LOGGING_NAME, "Unable to extract work contents from node: ", e.what());
//        return ExecStatus::INVALID_NODE;
//      }
//
//      // Getting the contract name
//      Identifier contract_id;
//      if (!contract_id.Parse(node.contract_name))
//      {
//        return ExecStatus::CONTRACT_NAME_PARSE_FAILURE;
//      }
//
//      // create the resource address for the contract
//      auto const contract_resource = SmartContractManager::CreateAddressForContract(contract_id);
//
//      // lookup the contract document from the storage engine
//      auto const contract_document = storage_.Get(contract_resource);
//
//      if (contract_document.failed)
//      {
//        FETCH_LOG_WARN(LOGGING_NAME, "Failed to lookup the contract from requested address");
//        return ExecStatus::INVALID_CONTRACT_ADDRESS;
//      }

//      // duplicate possible?
//      auto it = synergetic_work_candidates.find(contract_resource.id());
//      if (it == synergetic_work_candidates.end())
//      {
//        synergetic_work_candidates[contract_resource.id()] = std::make_shared<WorkPackage>()
//            WorkPackage::New(node.contract_name, contract_address);
//      }
//
//      auto pack = synergetic_work_candidates[contract_address];
//      pack->solutions_queue.push(work);
//    }
//  }

//  // Move work to a priority queue
//  for (auto const &pack : synergetic_work_candidates)
//  {
//    contract_queue_.push_back(pack.second);
//  }
//
//  // Ensuring that tasks are consistently ranked across nodes
//  std::sort(contract_queue_.begin(), contract_queue_.end(),
//            [](SharedWorkPackage const &ptr1, SharedWorkPackage const &ptr2) {
//              return (*ptr1) < (*ptr2);
//            });
//
//  // Fetching the contracts
//  for (auto &c : contract_queue_)
//  {
//    if (!RegisterContract(c->contract_name, c->contract_address))
//    {
//      return PreparationStatusType::CONTRACT_REGISTRATION_FAILED;
//    }
//  }
//
//  return PreparationStatusType::SUCCESS;
  return SUCCESS;
}

bool SynergeticExecutionManager::ValidateWorkAndUpdateState()
{
//  // For each contract that was referenced by work in the DAG, we
//  // perform an update
//  for (auto &c : contract_queue_)
//  {
//    miner_.AttachContract(storage_, contract_register_.GetContract(c->contract_name));
//
//    // Defining the problem using the data on the DAG
//    if (!miner_.DefineProblem())
//    {
//      return false;
//    }
//
//    // Finding the best work
//    while (!c->solutions_queue.empty())
//    {
//      // Work is taking from a queue starting with the most promising
//      // work to avoid wasting time on verify all work.
//      auto work = c->solutions_queue.top();
//      c->solutions_queue.pop();
//      assert(work.miner != "");
//
//      // Verifying the score
//      Work::ScoreType score = miner_.ExecuteWork(work);
//
//      if (score == work.score)
//      {
//        // Work was honest
//        miner_.ClearContest();
//
//        // In case we are making a reward scheme for work, we emit
//        // a reward signal.
//        if (on_reward_work_)
//        {
//          on_reward_work_(work);
//        }
//
//        // We are done for this contract and can move on to the next one
//        break;
//      }
//      else
//      {
//        // In case the work was dishonest, we emit a dishonest work signal
//        // which can be used to update trust models.
//        if (on_dishonest_work_)
//        {
//          on_dishonest_work_(work);
//        }
//      }
//    }
//
//    miner_.DetachContract();
//  }

  return true;
}

} // namespace ledger
} // namespace fetch
