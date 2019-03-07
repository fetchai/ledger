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
  SynergeticMiner(fetch::ledger::DAG &dag)
  : dag_(dag)
  {
    CreateConensusVMModule(module_);

    // Preparing VM & compiler
    vm_       = new fetch::vm::VM(&module_);
    vm_->RegisterGlobalPointer(&dag_);
  }

  ~SynergeticMiner() 
  {
    delete vm_;
  }

  bool DefineProblem(SynergeticContract contract, Work const &work) 
  { 
    // Defining problem
    if (!vm_->Execute(contract->script, contract->problem_function, error_, problem_))
    {
      std::cerr << "Runtime error: " << error_ << std::endl;
      return false;
    }
    return true;
  }

  double ExecuteWork(SynergeticContract contract, Work work) 
  { 
    // Executing the work function
    int64_t nonce = work();
    if (!vm_->Execute(contract->script, contract->work_function, error_, solution_, problem_, nonce))
    {
      std::cerr << "Runtime error: " << error_ << std::endl;
      return std::numeric_limits< double >::infinity();
    }

    // Computing objective function
    if (!vm_->Execute(contract->script, contract->objective_function, error_, score_, problem_, solution_))
    {
      std::cerr << "Runtime error: " << error_ << std::endl;      
      return std::numeric_limits< double >::infinity(); 
    }

    // TODO: validate that it is i64
    
    return score_.primitive.f64; // TODO: Migrate to i64 and fixed points
  }
private:
  fetch::ledger::DAG  &dag_;
  fetch::vm::Module    module_;
  fetch::vm::VM *      vm_;

  std::string        error_;
  fetch::vm::Variant problem_;
  fetch::vm::Variant solution_; 
  fetch::vm::Variant score_;  
};


}
}