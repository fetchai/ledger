#pragma once

#include "ledger/upow/synergetic_contract_register.hpp"
#include "ledger/upow/work.hpp"
#include "ledger/upow/work_register.hpp"
#include "ledger/upow/synergetic_miner.hpp"
#include "ledger/upow/synergetic_vm_module.hpp"

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
  ContractAddress address{};
  WorkQueue       solutions_queue{};
  /// }

  bool operator<(WorkPackage const &other) const 
  {
    return address < other.address;
  }

private:
  WorkPackage(ContractAddress contract_address) : address{std::move(contract_address)} { }
};


class SynergeticExecutor 
{
public:
  using ContractAddress   = byte_array::ConstByteArray;  
  using SharedWorkPackage = std::shared_ptr< WorkPackage >;
  using ExecutionQueue    = std::vector< SharedWorkPackage >;
  using WorkMap           = std::unordered_map< ContractAddress, SharedWorkPackage >;
  using DAG = ledger::DAG;

  SynergeticExecutor(DAG &dag)
  : synergetic_miner_{dag}
  , dag_{dag}
  {

  }

  bool PrepareWorkQueue(Block &block)
  {    
    WorkMap synergetic_work_candidates;
    // TODO: contract_register_.Clear();

    // Traverse DAG last segment and find work
    for(auto &node: dag_.ExtractSegment(block.body.block_number))
    {
      if(node.type == WORK) 
      {
        // Deserialising work and adding it to the work map
        Work work;

        try
        {
          // TODO: Deserialise
        }
        catch(std::exception const &e)
        {
          return false;
        }

        // Adding the work to 
        auto it = synergetic_work_candidates.find(work.contract_address);
        if(it == synergetic_work_candidates.end())
        {
          synergetic_work_candidates[work.contract_address] = WorkPackage::New(work.contract_address);
        }

        auto pack = synergetic_work_candidates[work.contract_address];
        pack->solutions_queue.push_back(work);
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

      if(!contract_register_.CreateContract(c->contract_address, source))
      {
        return false;
      }      

    }



    return true;
  }

  bool ValidateWorkAndUpdateState()
  {
    for(auto &c: contract_queue_)
    {
      //
      fetch::consensus::WorkRegister work_register;

      // Finding the best work
      while(!c.solutions_queue.empty())
      {
        auto work = c.solutions_queue.front();
        Work::ScoreType score = work.score;

        c.solutions_queue.pop_front();

        // Defining the problem using the data on the DAG
        // TODO: should be moved outside of the while loop - not sure why the work is there
        if(!miner_.DefineProblem(contract_register_.GetContract(work.contract_address), work))
        {
          return false;
        }

        work.score = miner_.ExecuteWork(contract_register_.GetContract(work.contract_address), work);

        if(score == work.score)
        {

          
        }
      }
    }
  }

private:
  SynergeticContractRegister contract_register_;  ///< Cache for synergetic contracts
  SynergeticMiner            miner_;              ///< Miner to validate the work
  DAG &                      dag_;                           ///< DAG with work to be executed

  ExecutionQueue contract_queue_;
};

}
}
