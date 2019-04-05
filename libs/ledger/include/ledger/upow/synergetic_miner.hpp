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
  using ScoreType          = Work::ScoreType;
  using DAGNode            = ledger::DAGNode;
  using ChainState           = vm_modules::ChainState;    
  using UniqueStateAdapter = std::unique_ptr< SynergeticStateAdapter >;
  using Identifier         = ledger::Identifier;
  using StorageInterface   = ledger::StorageInterface;
  using BigUnsigned        = math::BigUnsigned;

  SynergeticMiner(fetch::ledger::DAG &dag)
  : dag_(dag)
  {
    CreateConensusVMModule(module_);

    // Preparing VM & compiler
    vm_             = new fetch::vm::VM(&module_);
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
    }

    // Defining problem
    if (!vm_->Execute(contract_->script, contract_->problem_function, error_, problem_))
    {
      std::cerr << "DefineProblem - TODO - work out how to propagate error" << std::endl;                  
      std::cout << "Failed to execute problem function: " << error_ << std::endl;
      return false;
    }

    return true;
  }

  ScoreType ExecuteWork(Work work) 
  { 
    assert(contract_ != nullptr);    
//    assert(work.block_number == chain_state_.block.body.block_number);

    if(has_state())
    {
      vm_->SetIOObserver(read_only_state());
    }
    else
    {
      vm_->ClearIOObserver();
    }

    if(work.contract_name != contract_->name)
    {
      throw std::runtime_error("Contract name between work and used contract differs: "+ static_cast<std::string>(work.contract_name) + " vs " + static_cast<std::string>(contract_->name) );
    }

    // Executing the work function
    auto hashed_nonce = vm_->CreateNewObject< vm_modules::BigNumberWrapper >( work() );

    if (!vm_->Execute(contract_->script, contract_->work_function, error_,  solution_, problem_, hashed_nonce))
    {
      std::cerr << "Runtime error: " << error_ << std::endl;
      vm_->ClearIOObserver();
      return std::numeric_limits< ScoreType >::max();
    }

    // Computing objective function
    if (!vm_->Execute(contract_->script, contract_->objective_function, error_, score_,  problem_, solution_))
    {
      std::cerr << "ExecuteWork - TODO - work out how to propagate error" << std::endl;            
      std::cerr << "Runtime error: " << error_ << std::endl;
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
    }

    // Invoking the clear function
    fetch::vm::Variant output;
    if (!vm_->Execute(contract_->script, contract_->clear_function, error_,  output, problem_, solution_))
    {
      std::cerr << "ClearContest - TODO - work out how to propagate error" << std::endl;      
      std::cerr << "Runtime error during clear: " << error_ << std::endl;
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



    if (!vm_->Execute(contract_->script, contract_->test_dag_generator, error_, new_dag_node, epoch, entropy))
    {
      std::cerr << "CreateDAGTestData - TODO - work out how to propagate error" << std::endl;
      std::cerr << "Runtime error: " << error_ << std::endl;
      return DAGNode();
    }

    if(new_dag_node.type_id != vm_->GetTypeId<vm_modules::DAGNodeWrapper >())
    {
      std::cerr << "TODO: Expected DAG node" << std::endl;
      return DAGNode();
    }

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

  ChainState chain_state_;
};


}
}