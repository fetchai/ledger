#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"
#include "crypto_rng.hpp"
#include "exp.hpp"
#include "byte_array_wrapper.hpp"
#include "print.hpp"
#include "item.hpp"
#include "dummy.hpp"
#include "dag_accessor.hpp"


namespace fetch
{
namespace consensus
{

class Miner {
public:
  Miner(fetch::ledger::DAG &dag)
  : dag_(dag)
  {
    // Creating miner module
    fetch::modules::CryptoRNG::Bind(module_);
    fetch::modules::ByteArrayWrapper::Bind(module_);
    fetch::modules::ItemWrapper::Bind(module_);
    fetch::modules::DAGWrapper::Bind(module_);

    fetch::modules::BindExp(module_);
    fetch::modules::BindPrint(module_);

    // Preparing VM & compiler
    compiler_ = new fetch::vm::Compiler(&module_);
    vm_       = new fetch::vm::VM(&module_);
    vm_->RegisterGlobalPointer(&dag_);
  }

  ~Miner() 
  {
    delete compiler_;
    delete vm_;
  }

  bool AttachContract(std::string address, std::string const &contract)
  {
    work_function_ = "";
    objective_function_ = "";
    fetch::vm::Strings errors;

    // Compiling contract
    if(!compiler_->Compile(contract, address, script_, errors))
    {
      // TODO: Get rid off error message
      std::cout << "Failed to compile" << std::endl;
      for (auto &s : errors)
      {
        std::cout << s << std::endl;
      }

      return false;
    }

    // Finding work and objective functions
    for(auto &f: script_.functions)
    {
      bool is_work = false;
      bool is_objective = false;    
      bool is_problem = false;

      for(auto &a : f.annotations)
      {
        is_work |= (a.name == "@work"); 
        is_objective |= (a.name == "@objective");     
        is_problem |= (a.name == "@problem");             
      }

      if(is_work)
      {
        work_function_ = f.name;
      }

      if(is_objective)
      {
        objective_function_ = f.name;
      }

      if(is_problem)
      {
        problem_function_ = f.name;
      }      
    }

    if(work_function_ == "")
    {
      return false;
    }

    if(objective_function_ == "")
    {
      return false;
    }

    if(problem_function_ == "")
    {
      std::cout << "Problem function not found!" << std::endl;
      return false;
    }    

    return true;
  }

  double ExecuteWork(int64_t nonce) 
  { 
    std::string        error;

    // Defining problem
    fetch::vm::Variant problem; 
    if (!vm_->Execute(script_, problem_function_, error, problem))
    {
      std::cout << "Runtime error 1: " << error << std::endl;
      return std::numeric_limits< double >::infinity(); 
    }

    // Executing the work function
    fetch::vm::Variant work; 
    if (!vm_->Execute(script_, work_function_, error, work, problem, nonce))
    {
      std::cout << "Runtime error 2: " << error << std::endl;
      return std::numeric_limits< double >::infinity(); 
    }

    // Computing objective function
    fetch::vm::Variant output;    
    if (!vm_->Execute(script_, objective_function_, error, output, problem, work))
    {
      std::cout << "Runtime error 3: " << error << std::endl;
    }

    // Storing work
    // TODO

    // TODO: validate
    return output.primitive.f64;
  }
private:
  fetch::ledger::DAG &dag_;
  fetch::vm::Module module_;
  fetch::vm::Compiler *compiler_;
  fetch::vm::VM *      vm_;
  fetch::vm::Script  script_;

  std::string problem_function_ = "";
  std::string work_function_ = "";
  std::string objective_function_ = "";  
};


}
}