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

#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/upow/synergetic_contract_register.hpp"
#include "ledger/upow/synergetic_miner.hpp"
#include "ledger/upow/synergetic_vm_module.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_package.hpp"
#include "ledger/upow/work_register.hpp"

#include "ledger/chain/block.hpp"
#include "ledger/identifier.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <algorithm>
#include <queue>
namespace fetch {
namespace consensus {

class SynergeticExecutor
{
public:
  using Identifier           = ledger::Identifier;
  using ContractAddress      = storage::ResourceAddress;
  using ConstByteArray       = byte_array::ConstByteArray;
  using SharedWorkPackage    = std::shared_ptr<WorkPackage>;
  using ExecutionQueue       = std::vector<SharedWorkPackage>;
  using WorkMap              = std::unordered_map<ContractAddress, SharedWorkPackage>;
  using DAG                  = ledger::DAG;
  using Block                = ledger::Block;
  using StorageUnitInterface = ledger::StorageUnitInterface;
  using UniqueCertificate    = std::unique_ptr<fetch::crypto::ECDSASigner>;
  using DAGNode              = ledger::DAGNode;

  enum PreparationStatusType
  {
    SUCCESS,
    CONTRACT_NAME_PARSE_FAILURE,
    INVALID_CONTRACT_ADDRESS,
    INVALID_NODE,
    INVALID_BLOCK,
    CONTRACT_REGISTRATION_FAILED
  };

  SynergeticExecutor(DAG &dag, StorageUnitInterface &storage_unit)
    : miner_{dag}
    , dag_{dag}
    , storage_{storage_unit}
  {}

  void AttachStandardOutputDevice(std::ostream &device)
  {
    miner_.AttachStandardOutputDevice(device);
  }

  void DetachStandardOutputDevice()
  {
    miner_.DetachStandardOutputDevice();
  }

  PreparationStatusType PrepareWorkQueue(Block const &previous, Block const &current)
  {
    contract_queue_.clear();
    WorkMap synergetic_work_candidates;

    // Annotating nodes in the DAG
    if (!dag_.SetNodeTime(current))
    {
      return PreparationStatusType::INVALID_BLOCK;
    }

    // Work is certified by current block
    auto segment = dag_.ExtractSegment(current);

    // but data is used from the previous block
    miner_.SetBlock(previous);
    assert((previous.body.block_number + 1) == current.body.block_number);

    // Extracting relevant contract names
    for (auto &node : segment)
    {
      if (node.type == ledger::DAGNode::WORK)
      {
        // Deserialising work and adding it to the work map
        Work work;
        try
        {
          node.GetObject(work);
          work.contract_name = node.contract_name;
          work.miner         = node.identity.identifier();
          assert((current.body.block_number - 1) == work.block_number);
        }
        catch (std::exception const &e)
        {
          return PreparationStatusType::INVALID_NODE;
        }

        // Getting the contract name
        Identifier contract_id;
        if (!contract_id.Parse(node.contract_name))
        {
          return PreparationStatusType::CONTRACT_NAME_PARSE_FAILURE;
        }

        // create the resource address for the contract
        ContractAddress contract_address =
            ledger::SmartContractManager::CreateAddressForContract(contract_id);

        auto it = synergetic_work_candidates.find(contract_address);
        if (it == synergetic_work_candidates.end())
        {
          synergetic_work_candidates[contract_address] =
              WorkPackage::New(node.contract_name, contract_address);
        }

        auto pack = synergetic_work_candidates[contract_address];
        pack->solutions_queue.push(work);
      }
    }

    // Move work to a priority queue
    for (auto const &pack : synergetic_work_candidates)
    {
      contract_queue_.push_back(pack.second);
    }

    // Ensuring that tasks are consistently ranked across nodes
    std::sort(contract_queue_.begin(), contract_queue_.end(),
              [](SharedWorkPackage const &ptr1, SharedWorkPackage const &ptr2) {
                return (*ptr1) < (*ptr2);
              });

    // Fetching the contracts
    for (auto &c : contract_queue_)
    {
      if (!RegisterContract(c->contract_name, c->contract_address))
      {
        return PreparationStatusType::CONTRACT_REGISTRATION_FAILED;
      }
    }

    return PreparationStatusType::SUCCESS;
  }

  bool ValidateWorkAndUpdateState()
  {
    // For each contract that was referenced by work in the DAG, we
    // perform an update
    for (auto &c : contract_queue_)
    {
      miner_.AttachContract(storage_, contract_register_.GetContract(c->contract_name));

      // Defining the problem using the data on the DAG
      if (!miner_.DefineProblem())
      {
        return false;
      }

      // Finding the best work
      while (!c->solutions_queue.empty())
      {
        // Work is taking from a queue starting with the most promising
        // work to avoid wasting time on verify all work.
        auto work = c->solutions_queue.top();
        c->solutions_queue.pop();
        assert(work.miner != "");

        // Verifying the score
        Work::ScoreType score = miner_.ExecuteWork(work);

        if (score == work.score)
        {
          // Work was honest
          miner_.ClearContest();

          // In case we are making a reward scheme for work, we emit
          // a reward signal.
          if (on_reward_work_)
          {
            on_reward_work_(work);
          }

          // We are done for this contract and can move on to the next one
          break;
        }
        else
        {
          // In case the work was dishonest, we emit a dishonest work signal
          // which can be used to update trust models.
          if (on_dishonest_work_)
          {
            on_dishonest_work_(work);
          }
        }
      }

      miner_.DetachContract();
    }

    return true;
  }

  DAGNode SubmitWork(UniqueCertificate &certificate, Work const &work)
  {
    fetch::ledger::DAGNode node;
    node.type = DAGNode::WORK;

    // Creating node details
    node.SetObject(work);
    node.contract_name = work.contract_name;
    node.identity      = certificate->identity();
    dag_.SetNodeReferences(node);

    // Signing
    node.Finalise();
    if (!certificate->Sign(node.hash))
    {
      return DAGNode();
    }

    node.signature = certificate->signature();

    // Pushing
    if (!dag_.Push(node))
    {
      return DAGNode();
    }

    return node;
  }

  Work Mine(UniqueCertificate &certificate, byte_array::ConstByteArray contract_name,
            Block &latest_block, uint64_t nonce_start = 23273, int32_t search_length = 10)
  {
    // Preparing the work meta data
    fetch::consensus::Work work;
    work.contract_name = std::move(contract_name);
    work.miner         = certificate->identity().identifier();
    work.block_number  = latest_block.body.block_number;

    // Getting or creating contract
    auto contract = contract_register_.GetContract(work.contract_name);
    if (contract == nullptr)
    {
      // Getting the contract name
      Identifier contract_id;
      if (!contract_id.Parse(work.contract_name))
      {
        return Work();
      }

      // create the resource address for the contract
      ContractAddress contract_address =
          ledger::SmartContractManager::CreateAddressForContract(contract_id);
      if (!RegisterContract(contract_name, contract_address))
      {
        return Work();
      }

      contract = contract_register_.GetContract(work.contract_name);
    }

    // Attaching the contract
    if (!miner_.AttachContract(storage_, contract))
    {
      return Work();
    }

    // Ensuring that the DAG is correctly certified and that
    // we are extracting the right part of the DAG
    miner_.SetBlock(latest_block);
    dag_.SetNodeTime(latest_block);

    // Defining the problem we mine
    if (!miner_.DefineProblem())
    {
      miner_.DetachContract();
      return Work();
    }

    // Preparing to mine
    fetch::math::BigUnsigned       nonce(nonce_start);
    fetch::consensus::WorkRegister work_register;

    // ... and mining
    for (int64_t i = 0; i < search_length; ++i)
    {
      work.nonce = nonce.Copy();
      work.score = miner_.ExecuteWork(work);

      ++nonce;
      work_register.RegisterWork(work);
    }

    miner_.DetachContract();

    // Returning the best work from this round
    return work_register.ClearWorkPool(contract);
  }

  void OnDishonestWork(std::function<void(Work)> on_dishonest_work)
  {
    on_dishonest_work_ = std::move(on_dishonest_work);
  }

  void OnRewardWork(std::function<void(Work)> on_reward_work)
  {
    on_reward_work_ = std::move(on_reward_work);
  }

  std::vector<std::string> const &errors() const
  {
    return miner_.errors();
  }

private:
  bool RegisterContract(byte_array::ConstByteArray contract_name, ContractAddress contract_address)
  {
    // Getting contract source and registering the contract
    auto const result = storage_.Get(contract_address);

    // Invalid address
    if (result.failed)
    {
      return false;
    }

    // Extracting the source
    ConstByteArray               source;
    serializers::ByteArrayBuffer adapter{result.document};
    adapter >> source;

    // Registering contract
    if (!contract_register_.CreateContract(contract_name, static_cast<std::string>(source)))
    {
      return false;
    }
    return true;
  }

  SynergeticMiner       miner_;  ///< Miner to validate the work
  DAG &                 dag_;    ///< DAG with work to be executed
  StorageUnitInterface &storage_;

  std::function<void(Work)> on_dishonest_work_;  ///< Callback to handle dishonest work
  std::function<void(Work)> on_reward_work_;     ///< Callback for rewards for accepted work

  SynergeticContractRegister contract_register_;  ///< Cache for synergetic contracts
  ExecutionQueue             contract_queue_;     ///< Queue for contracts invoked in an epoch
};

}  // namespace consensus
}  // namespace fetch
