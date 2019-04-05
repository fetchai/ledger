#pragma once
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/upow/synergetic_contract_register.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_register.hpp"
#include "ledger/upow/synergetic_miner.hpp"
#include "ledger/upow/synergetic_vm_module.hpp"

#include "ledger/chain/block.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/identifier.hpp"

#include <algorithm>
#include <queue>
namespace fetch
{
namespace consensus
{

struct WorkPackage
{
  using ContractName         = byte_array::ConstByteArray;
  using ContractAddress      = storage::ResourceAddress;  
  using SharedWorkPackage    = std::shared_ptr< WorkPackage >;  
  using WorkQueue            = std::priority_queue< Work >;
  

  static SharedWorkPackage New(ContractName name, ContractAddress contract_address) {
    return SharedWorkPackage(new WorkPackage(std::move(name), std::move(contract_address)));
  }

  /// @name defining work and candidate solutions
  /// @{
  ContractName    contract_name{};
  ContractAddress contract_address{};
  WorkQueue       solutions_queue{};
  /// }

  bool operator<(WorkPackage const &other) const 
  {
    return contract_address < other.contract_address;
  }

private:
  WorkPackage(ContractName name, ContractAddress address) 
  : contract_name{std::move(name)}
  , contract_address{std::move(address)} { }
};


class SynergeticExecutor 
{
public:
  using Identifier           = ledger::Identifier;
  using ContractAddress      = storage::ResourceAddress;
  using ConstByteArray       = byte_array::ConstByteArray;
  using SharedWorkPackage    = std::shared_ptr< WorkPackage >;
  using ExecutionQueue       = std::vector< SharedWorkPackage >;
  using WorkMap              = std::unordered_map< ContractAddress, SharedWorkPackage >;
  using DAG                  = ledger::DAG;
  using Block                = ledger::Block;
  using StorageUnitInterface = ledger::StorageUnitInterface;
  
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
  { }

  PreparationStatusType PrepareWorkQueue(Block const &previous, Block const &current)
  {
    contract_queue_.clear();
    WorkMap synergetic_work_candidates;

    // Annotating nodes in the DAG
    if(!dag_.SetNodeTime(current))
    {
      return PreparationStatusType::INVALID_BLOCK;
    }

    // Work is certified by current block
    auto segment = dag_.ExtractSegment(current);

    // but data is used from the previous block
    miner_.SetBlock(previous);
    assert((previous.body.block_number +1) == current.body.block_number );

    // Extracting relevant contract names
    for(auto &node: segment)
    {
      if(node.type == ledger::DAGNode::WORK) 
      {
        // Deserialising work and adding it to the work map
        Work work;
        try
        {
          node.GetObject(work);
          work.contract_name = node.contract_name;
          work.miner = node.identity.identifier();
          assert((current.body.block_number - 1) == work.block_number);
        }
        catch(std::exception const &e)
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
        ContractAddress contract_address = ledger::SmartContractManager::CreateAddressForContract(contract_id);

        auto it = synergetic_work_candidates.find(contract_address);
        if(it == synergetic_work_candidates.end())
        {
          synergetic_work_candidates[contract_address] = WorkPackage::New(node.contract_name, contract_address);
        }

        auto pack = synergetic_work_candidates[contract_address];
        pack->solutions_queue.push(work);

      }
    }

    // Move work to a priority queue
    for(auto const& pack: synergetic_work_candidates)
    {
      contract_queue_.push_back(pack.second);
    }

    // Ensuring that tasks are consistently ranked across nodes
    std::sort(contract_queue_.begin(), contract_queue_.end(), [](SharedWorkPackage const& ptr1, SharedWorkPackage const& ptr2) {
      return (*ptr1) < (*ptr2);
    });

    // Fetching the contracts
    for(auto &c: contract_queue_)
    {
      // Getting contract source and registering the contract
      auto const result = storage_.Get(c->contract_address);

      // Invalid address
      if (result.failed)
      {
        return PreparationStatusType::INVALID_CONTRACT_ADDRESS;
      }

      // Extracting the source 
      ConstByteArray source; 
      serializers::ByteArrayBuffer adapter{result.document};      
      adapter >> source;

      // Registering contract
      if(!contract_register_.CreateContract(c->contract_name, static_cast<std::string>(source) ))
      {
        return PreparationStatusType::CONTRACT_REGISTRATION_FAILED;
      }
    }

    return PreparationStatusType::SUCCESS;
  }

  bool ValidateWorkAndUpdateState()
  {
    for(auto &c: contract_queue_)      
    {
      miner_.AttachContract(storage_, contract_register_.GetContract(c->contract_name));

      // Defining the problem using the data on the DAG
      if(!miner_.DefineProblem())
      {
        return false;
      }

      // Finding the best work
      while(!c->solutions_queue.empty())
      {
        auto work = c->solutions_queue.top();
        assert( work.miner != "");

        c->solutions_queue.pop();

        Work::ScoreType score = miner_.ExecuteWork(work);
        if(score == work.score)
        {
          // Work was honest
          miner_.ClearContest();

          assert( on_reward_work_ );
          if(on_reward_work_)
          {
            on_reward_work_(work);
          }
          break;
        }
        else
        {
          if(on_dishonest_work_)
          {
            on_dishonest_work_(work);
          }
        }
      }

      miner_.DetachContract();
    }

    return true;
  }

  void OnDishonestWork(std::function< void(Work) > on_dishonest_work)
  {
    on_dishonest_work_ = std::move(on_dishonest_work);
  }
private:
  SynergeticContractRegister contract_register_;  ///< Cache for synergetic contracts
  SynergeticMiner            miner_;              ///< Miner to validate the work
  DAG &                      dag_;                ///< DAG with work to be executed
  StorageUnitInterface &     storage_;

  std::function< void(Work) > on_dishonest_work_;

  ExecutionQueue contract_queue_;
};

}
}
