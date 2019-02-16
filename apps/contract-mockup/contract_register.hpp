#pragma once

#include "vm/analyser.hpp"
#include "vm/typeids.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "synergetic_contract.hpp"

namespace fetch
{
namespace consensus
{


class ContractRegister
{
public:
  using ContractAddress = byte_array::ConstByteArray;

  ContractRegister()
  {
    CreateConensusVMModule(module_);    
    compiler_ = new fetch::vm::Compiler(&module_); // TODO: Make unique_ptr;
  }

  ~ContractRegister()
  {
    delete compiler_;
  }

  SynergeticContract AddContract(ContractAddress const &contract_address, std::string const &source)
  {
    SynergeticContract ret = NewSynergeticContract(compiler_, contract_address, source);
    contracts_[contract_address] = ret;
    return ret;
  }

  SynergeticContract GetContract(ContractAddress const &address)
  {
    auto it = contracts_.find(address);
    if(it == contracts_.end())
    {
      return nullptr;
    }
    return it->second;
  }
private:
  vm::Module     module_;
  vm::Compiler  *compiler_ = nullptr;
  std::unordered_map< ContractAddress, SynergeticContract > contracts_;
};

}
}