#pragma once

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "synergetic_vm_module.hpp"
#include "synergetic_contract.hpp"
#include "synergetic_state_adapter.hpp"
#include "work.hpp"
#include "math/bignumber.hpp"

#include "ledger/storage_unit/storage_unit_interface.hpp"

namespace fetch
{
namespace consensus
{

class SynergeticMiner {
public:
  using ScoreType            = Work::ScoreType;
  using DAGNode              = ledger::DAGNode;
  using ChainState           = vm_modules::ChainState;    
  using UniqueStateAdapter   = std::unique_ptr< SynergeticStateAdapter >;
  using Identifier           = ledger::Identifier;
  using StorageInterface     = ledger::StorageInterface;
  using BigUnsigned          = math::BigUnsigned;

  SynergeticMiner(fetch::ledger::DAG &dag)
  : dag_(dag)
  {
    CreateConensusVMModule(module_);

    // Preparing VM & compiler
    vm_               = new fetch::vm::VM(&module_);
    chain_state_.dag  =  &dag_;

    // TODO: Workout what to do with the stdout
    vm_->AttachOutputDevice("stdout", std::cout);
    vm_->RegisterGlobalPointer(&chain_state_);    
  }

  ~SynergeticMiner() 
  {
    delete vm_;
  }

  bool DefineProblem() 
  { 
    assert(contract_ != nullptr);

    if(has_state())
    {
      vm_->SetIOObserver(read_only_state());
    }
    else
    {
      vm_->ClearIOObserver();
      errors_.push_back("Could not attach state in clear contest.");
      return false;
    }

    // Defining problem
    if (!vm_->Execute(contract_->script, contract_->problem_function, error_, problem_))
    {
      errors_.push_back("Error while defining problem.");
      errors_.push_back(error_);
      return false;
    }

    return true;
  }

  ScoreType ExecuteWork(Work work) 
  { 
    assert(contract_ != nullptr);    
    assert(work.block_number == chain_state_.block.body.block_number);

    if(has_state())
    {
      vm_->SetIOObserver(read_only_state());
    }
    else
    {
      vm_->ClearIOObserver();
      errors_.push_back("Could not attach state in clear contest.");
      return false;
    }

    if(work.contract_name != contract_->name)
    {
      errors_.push_back("Contract name between work and used contract differs: "+ static_cast<std::string>(work.contract_name) + " vs " + static_cast<std::string>(contract_->name) );
      return std::numeric_limits< ScoreType >::max();
    }

    // Executing the work function
    auto hashed_nonce = vm_->CreateNewObject< vm_modules::BigNumberWrapper >( work() );

    if (!vm_->Execute(contract_->script, contract_->work_function, error_,  solution_, problem_, hashed_nonce))
    {
      errors_.push_back("Error while executing work function.");
      errors_.push_back(error_);

      vm_->ClearIOObserver();
      return std::numeric_limits< ScoreType >::max();
    }

    // Computing objective function
    if (!vm_->Execute(contract_->script, contract_->objective_function, error_, score_,  problem_, solution_))
    {
      errors_.push_back("Error while executing objective function for work.");
      errors_.push_back(error_);

      vm_->ClearIOObserver();      
      return std::numeric_limits< ScoreType >::max();
    }
    
    return score_.primitive.f64; // TODO: Migrate to i64 and fixed points
  }

  bool ClearContest()
  {
    assert(contract_ != nullptr);    
    if(has_state())
    {
      vm_->SetIOObserver(read_write_state());
    }
    else
    {
      vm_->ClearIOObserver();
      errors_.push_back("Could not attach state in clear contest.");
      return false;
    }

    // Invoking the clear function
    fetch::vm::Variant output;
    if (!vm_->Execute(contract_->script, contract_->clear_function, error_,  output, problem_, solution_))
    {
      errors_.push_back("Error while clearing contest during execution.");
      errors_.push_back(error_);      

      vm_->ClearIOObserver();
      return false;
    }


    return true;
  }

  DAGNode CreateDAGTestData(int32_t epoch, int64_t n_entropy)
  {
    assert(contract_ != nullptr);    
    fetch::vm::Variant new_dag_node;

    // Generating entropy
    fetch::crypto::SHA256 hasher;
    hasher.Update( n_entropy );
    auto entropy = vm_->template CreateNewObject< fetch::vm_modules::BigNumberWrapper >(hasher.Final());

    // Executing main routine for data creation
    if (!vm_->Execute(contract_->script, contract_->test_dag_generator, error_, new_dag_node, epoch, entropy))
    {
      errors_.push_back("Error during execution while creating DAG test data.");
      errors_.push_back(error_);

      return DAGNode();
    }

    // Checking that return type is good.
    if(new_dag_node.type_id != vm_->GetTypeId<vm_modules::DAGNodeWrapper >())
    {
      errors_.push_back("Return type error while creating DAG test data.");

      return DAGNode();
    }

    // Returning the result
    auto node_wrapper = new_dag_node.Get< vm::Ptr< vm_modules::DAGNodeWrapper > >();
    return node_wrapper->ToDAGNode();
  }


  bool AttachContract(StorageInterface &storage, SynergeticContract contract)
  {
    if(contract == nullptr)
    {
      return false;
    }
    contract_ = contract;
    Identifier contract_id;
    if (!contract_id.Parse(contract_->name))
    {
      return false;
    }

    read_only_state_.reset( new SynergeticStateAdapter(storage, contract_id) );
    read_write_state_.reset( new SynergeticStateAdapter(storage, contract_id, true) );
    return true;
  }

  bool AttachContract(SynergeticContract contract)
  {
    contract_ = contract;
    Identifier contract_id;
    if (!contract_id.Parse(contract_->name))
    {
      return false;
    }

    read_only_state_.reset( );
    read_write_state_.reset( );
    return true;
  }  

  void DetachContract()
  {
    vm_->ClearIOObserver();

    contract_.reset();
    read_only_state_.reset();
    read_write_state_.reset();
  }  

  void SetBlock(ledger::Block block)
  {
    chain_state_.block = std::move(block);
  }

  ledger::Block block()
  {
    return chain_state_.block;
  }

  template <typename T, typename... Args>
  vm::Ptr<T> CreateNewObject(Args &&... args)
  {
    return vm_->CreateNewObject<T>(std::forward<Args>(args)...);
  }

  typename ledger::DAG::NodeArray GetDAGSegment() const
  {
    return chain_state_.dag->ExtractSegment(chain_state_.block);
  }

  uint64_t block_number() const
  {
    return chain_state_.block.body.block_number;
  }

  std::vector< std::string > const &errors() const
  {
    return errors_;
  }  
private:
  bool has_state() const
  {
    return (read_only_state_!=nullptr) && (read_write_state_!=nullptr);
  }

  ledger::StateAdapter &read_only_state()
  {
    return *read_only_state_;
  }

  ledger::StateAdapter &read_write_state()
  {
    return *read_write_state_;
  }  

  fetch::ledger::DAG  &dag_;
  fetch::vm::Module    module_;
  fetch::vm::VM *      vm_;

  std::string        error_;
  fetch::vm::Variant problem_;
  fetch::vm::Variant solution_; 
  fetch::vm::Variant score_;  
  std::string text_output_;
  SynergeticContract contract_{nullptr};

  UniqueStateAdapter read_only_state_{nullptr};
  UniqueStateAdapter read_write_state_{nullptr};

  std::vector< std::string > errors_;

  ChainState chain_state_;
};


}
}