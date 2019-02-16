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

  double ExecuteWork(SynergeticContract contract, Work work) 
  { 
    std::string        error;

    // Defining problem
    fetch::vm::Variant problem; 
    if (!vm_->Execute(contract->script, contract->problem_function, error, problem))
    {
      std::cout << "Runtime error 1: " << error << std::endl;
      return std::numeric_limits< double >::infinity(); 
    }

    // Executing the work function
    fetch::vm::Variant solution; 
    int64_t nonce = work();
    if (!vm_->Execute(contract->script, contract->work_function, error, solution, problem, nonce))
    {
      std::cout << "Runtime error 2: " << error << std::endl;
      return std::numeric_limits< double >::infinity(); 
    }

    // Computing objective function
    fetch::vm::Variant output;    
    if (!vm_->Execute(contract->script, contract->objective_function, error, output, problem, solution))
    {
      std::cout << "Runtime error 3: " << error << std::endl;
      return std::numeric_limits< double >::infinity(); 
    }

    // Storing work
    // TODO

    // TODO: validate
    return output.primitive.f64;
  }
private:
  fetch::ledger::DAG  &dag_;
  fetch::vm::Module    module_;
  fetch::vm::VM *      vm_;
};


}
}