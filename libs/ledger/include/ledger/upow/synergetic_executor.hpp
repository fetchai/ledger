#pragma once

#include "ledger/upow/synergetic_contract_register.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_register.hpp"
#include "ledger/upow/synergetic_miner.hpp"
#include "ledger/upow/synergetic_vm_module.hpp"

#include "ledger/chain/block.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <algorithm>
#include <queue>
namespace fetch
{
namespace consensus
{

struct WorkPackage
{
  using ContractAddress   = byte_array::ConstByteArray;  
  using SharedWorkPackage = std::shared_ptr< WorkPackage >;  
  using WorkQueue = std::priority_queue< Work >;
  

  static SharedWorkPackage New(ContractAddress contract_address) {
    return SharedWorkPackage(new WorkPackage(std::move(contract_address)));
  }

  /// @name defining work and candidate solutions
  /// @{
  ContractAddress contract_address{};
  WorkQueue       solutions_queue{};
  /// }

  bool operator<(WorkPackage const &other) const 
  {
    return contract_address < other.contract_address;
  }

private:
  WorkPackage(ContractAddress address) : contract_address{std::move(address)} { }
};


class SynergeticExecutor 
{
public:
  using ConstByteArray    = byte_array::ConstByteArray;
  using ContractAddress   = byte_array::ConstByteArray;  
  using SharedWorkPackage = std::shared_ptr< WorkPackage >;
  using ExecutionQueue    = std::vector< SharedWorkPackage >;
  using WorkMap           = std::unordered_map< ContractAddress, SharedWorkPackage >;
  using DAG               = ledger::DAG;
  using Block             = ledger::Block;
  using StorageUnitInterface = ledger::StorageUnitInterface;
  
  enum StatusType 
  {
    SUCCESS,
    CONTRACT_NAME_PARSE_FAILURE,
    INVALID_NODE
  };

  SynergeticExecutor(DAG &dag, StorageUnitInterface &storage_unit)
  : miner_{dag}
  , dag_{dag}
  , storage_unit_{storage_unit}
  {
    (void)dag_;
    (void)storage_unit_;
  }

  StatusType PrepareWorkQueue(Block &block)
  {
    return StatusType::SUCCESS;
//    WorkMap synergetic_work_candidates;
    // TODO: contract_register_.Clear();
/*
    // Traverse DAG last segment and find work
    for(auto &node: dag_.ExtractSegment(block.body.block_number))
    {
      if(node.type == ledger::DAGNode::WORK) 
      {
        // Deserialising work and adding it to the work map
        Work work;

        try
        {
          // TODO: Deserialise
          serializers::TypedByteArrayBuffer buf(node.contents);
          buf >> ret;
        }
        catch(std::exception const &e)
        {
          return INVALID_NODE;
        }

        Identifier contract_id;
        if (!contract_id.Parse(node.contract_name)
        {
          return Status::CONTRACT_NAME_PARSE_FAILURE;
        }

        // Adding the work to 
        ConstByteArray contract_address = ""; 

        auto it = synergetic_work_candidates.find(contract_address);
        if(it == synergetic_work_candidates.end())
        {
          synergetic_work_candidates[contract_address] = WorkPackage::New(contract_address);
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
      ConstByteArray source = "";  // TODO: Wait for smart contracts to be finished

      if(!contract_register_.CreateContract(c->contract_address, static_cast<std::string>(source) ))
      {
        return false;
      }      

    }

    return true;
    */
  }

  bool ValidateWorkAndUpdateState()
  {
    /*
    for(auto &c: contract_queue_)
    {
      // Defining the problem using the data on the DAG
      if(!miner_.DefineProblem(contract_register_.GetContract(c->contract_address)))
      {
        return false;
      }

      // Finding the best work
      while(!c->solutions_queue.empty())
      {
        auto work = c->solutions_queue.top();
        c->solutions_queue.pop();

        Work::ScoreType score = miner_.ExecuteWork(contract_register_.GetContract(work.contract_address), work);
        if(score == work.score)
        {
          // TODO: Work was honest and we just need to update the state
          // Need contract implementation
          break;
        }
        else
        {
          // Work was dishonst and we slash stake
          // TODO.
        }
      }
    }
*/
    return true;
  }

private:
  SynergeticContractRegister contract_register_;  ///< Cache for synergetic contracts
  SynergeticMiner            miner_;              ///< Miner to validate the work
  DAG &                      dag_;                           ///< DAG with work to be executed
  StorageUnitInterface &     storage_unit_;

  ExecutionQueue contract_queue_;
};

}
}
