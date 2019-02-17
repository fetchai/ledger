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

class SynergeticContractRegister
{
public:
  using ContractAddress = byte_array::ConstByteArray;

  SynergeticContractRegister()
  {
    CreateConensusVMModule(module_);    
    compiler_ = new fetch::vm::Compiler(&module_); // TODO: Make unique_ptr;
  }

  ~SynergeticContractRegister()
  {
    delete compiler_;
  }

  SynergeticContract CreateContract(ContractAddress const &contract_address, std::string const &source)
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