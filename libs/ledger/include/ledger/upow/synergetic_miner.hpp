#pragma once

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "synergetic_vm_module.hpp"
#include "synergetic_contract.hpp"
#include "work.hpp"

namespace fetch
{
namespace consensus
{

class SynergeticMiner {
public:
  using ScoreType = Work::ScoreType;
  using DAGNode = ledger::DAGNode;

  SynergeticMiner(fetch::ledger::DAG &dag)
  : dag_(dag)
  {
    CreateConensusVMModule(module_);

    // Preparing VM & compiler
    vm_       = new fetch::vm::VM(&module_);
    vm_->AttachOutputDevice("stdout", std::cout);
    vm_->RegisterGlobalPointer(&dag_);
  }

  ~SynergeticMiner() 
  {
    delete vm_;
  }

  bool DefineProblem(SynergeticContract contract) 
  { 
    assert(contract != nullptr);

    // Defining problem
    if (!vm_->Execute(contract->script, contract->problem_function, error_, problem_))
    {
      std::cout << "Failed to execute problem function: " << error_ << std::endl;
      return false;
    }
    return true;
  }

  ScoreType ExecuteWork(SynergeticContract contract, Work work) 
  { 

    if(work.contract_name != contract->name)
    {
      throw std::runtime_error("Contract name between work and used contract differs.");
    }

    // Executing the work function
    int64_t nonce = work();

    if (!vm_->Execute(contract->script, contract->work_function, error_,  solution_, problem_, nonce))
    {
      std::cerr << "Runtime error: " << error_ << std::endl;
      return std::numeric_limits< ScoreType >::max();
    }

    // Computing objective function
    if (!vm_->Execute(contract->script, contract->objective_function, error_, score_,  problem_, solution_))
    {
      std::cerr << "Runtime error: " << error_ << std::endl;
      return std::numeric_limits< ScoreType >::max();
    }
    
    return score_.primitive.f64; // TODO: Migrate to i64 and fixed points
  }

  DAGNode CreateDAGTestData(SynergeticContract contract, int32_t epoch, int32_t entropy)
  {
    fetch::vm::Variant new_dag_node;

    if (!vm_->Execute(contract->script, contract->test_dag_generator, error_, new_dag_node, epoch, entropy))
    {
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
private:
  fetch::ledger::DAG  &dag_;
  fetch::vm::Module    module_;
  fetch::vm::VM *      vm_;

  std::string        error_;
  fetch::vm::Variant problem_;
  fetch::vm::Variant solution_; 
  fetch::vm::Variant score_;  
  std::string text_output_;
};


}
}