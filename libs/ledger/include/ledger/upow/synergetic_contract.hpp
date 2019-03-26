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
  using SynergeticContract = std::shared_ptr< SynergeticContractClass >;

  static SynergeticContract New(vm::Compiler *compiler, byte_array::ByteArray const & name, std::string const &source)
  {
    if(source.empty())
    {
      std::cout << "No source present" << std::endl;
      return nullptr;
    }
    
    SynergeticContract ret = std::make_shared<SynergeticContractClass>();
    ret->name = name;
    // TODO: Compute address
    // ret->address = TODO.    
    ret->work_function = "";
    ret->objective_function = "";
    ret->problem_function = "";

    // TODO: Expose errors externally - possibly throw exception?
    fetch::vm::Strings errors;

    // Compiling contract
    std::string str_address = std::string( name );
    if(!compiler->Compile(source, str_address , ret->script, errors))
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
      bool is_clear_function = false;      
      bool is_test_dag_generator = false;

      for(auto &a : f.annotations)
      {
        is_work |= (a.name == "@work"); 
        is_objective |= (a.name == "@objective");     
        is_problem |= (a.name == "@problem");
        is_clear_function |= (a.name == "@clear");        
        is_test_dag_generator |= (a.name == "@test_dag_generator");        
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

      if(is_test_dag_generator)
      {
        ret->test_dag_generator = f.name;
      }      
    }

    if(ret->work_function == "")
    {
      std::cout << "Failed to find work function" << std::endl;            
      return nullptr;
    }

    if(ret->objective_function == "")
    {
      std::cout << "Failed to find objective function" << std::endl;      
      return nullptr;
    }

    if(ret->problem_function == "")
    {
      std::cout << "Failed to find problem function" << std::endl;
      return nullptr;
    }    

    //

    return ret;
  }


  byte_array::ByteArray name;   //< TODO: Set
  byte_array::ByteArray address;
  vm::Script script;

  // TODO: Extend such that each contract can have multiple triplets of these  
  std::string problem_function{""};
  std::string work_function{""};
  std::string objective_function{""};
  std::string clear_function{""};   

  std::string test_dag_generator{""};

  int64_t clear_interval{1};
};

using SynergeticContract = SynergeticContractClass::SynergeticContract;




}
}