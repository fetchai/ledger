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
/**
 * Thoughts on VM:
 *   - Can I pass variants from one VM to another safely?
 **/
class Miner {
public:
  Miner(fetch::ledger::DAG &dag)
  : dag_(dag)
  {
    CreateConensusVMModule(module_);

    // Preparing VM & compiler
    vm_       = new fetch::vm::VM(&module_);
    vm_->RegisterGlobalPointer(&dag_);
  }

  ~Miner() 
  {
    delete vm_;
  }

  bool DefineProblem(SynergeticContract contract, Work const &work) 
  { 
    // Defining problem
    if (!vm_->Execute(contract->script, contract->problem_function, error_, problem_))
    {
      return false;
    }
    return true;
  }

  int64_t ExecuteWork(SynergeticContract contract, Work work) 
  { 
    // Executing the work function
    int64_t nonce = work();
    if (!vm_->Execute(contract->script, contract->work_function, error_, solution_, problem_, nonce))
    {
      return std::numeric_limits< int64_t >::infinity();
    }

    // Computing objective function
    if (!vm_->Execute(contract->script, contract->objective_function, error_, score_, problem_, solution_))
    {
      return std::numeric_limits< int64_t >::infinity(); 
    }

    // TODO: validate that it is i64
    
    return score_.primitive.i64;
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