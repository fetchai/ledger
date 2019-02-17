#pragma once

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include <memory>

namespace fetch
{
namespace consensus
{

struct SynergeticContractClass
{
  byte_array::ByteArray address;
  vm::Script script;

  // TODO: Extend such that each contract can have multiple triplets of these  
  std::string problem_function{""};
  std::string work_function{""};
  std::string objective_function{""};
  std::string clear_function{""};   
  int64_t clear_interval{1};
};

using SynergeticContract = std::shared_ptr< SynergeticContractClass >;

static SynergeticContract NewSynergeticContract(vm::Compiler *compiler, byte_array::ByteArray const & address, std::string const &contract)
{
  SynergeticContract ret = std::make_shared<SynergeticContractClass>();

  ret->address = address;
  ret->work_function = "";
  ret->objective_function = "";
  ret->problem_function = "";

  // TODO: Expose errors externally - possibly throw exception?
  fetch::vm::Strings errors;

  // Compiling contract
  std::string str_address = std::string( byte_array::ToBase64(address) );
  if(!compiler->Compile(contract, str_address , ret->script, errors))
  {
    // TODO: Get rid off error message
    std::cout << "Failed to compile" << std::endl;
    for (auto &s : errors)
    {
      std::cout << s << std::endl;
    }

    return nullptr;
  }

  // Finding work and objective functions
  for(auto &f: ret->script.functions)
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
      ret->work_function = f.name;
    }

    if(is_objective)
    {
      ret->objective_function = f.name;
    }

    if(is_problem)
    {
      ret->problem_function = f.name;
    }      
  }

  if(ret->work_function == "")
  {
    std::cout << "Work function not found!" << std::endl;      
    return nullptr;
  }

  if(ret->objective_function == "")
  {
    std::cout << "Objective function not found!" << std::endl;
    return nullptr;
  }

  if(ret->problem_function == "")
  {
    std::cout << "Problem function not found!" << std::endl;
    return nullptr;
  }    

  return ret;
}


}
}