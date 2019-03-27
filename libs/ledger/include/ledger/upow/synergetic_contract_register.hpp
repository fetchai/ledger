#pragma once
#include "ledger/upow/synergetic_vm_module.hpp"

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
  using ContractName      = byte_array::ConstByteArray;

  SynergeticContractRegister()
  {
    CreateConensusVMModule(module_);    
    compiler_ = new fetch::vm::Compiler(&module_); // TODO: Make unique_ptr;
  }

  ~SynergeticContractRegister()
  {
    delete compiler_;
  }

  SynergeticContract CreateContract(ContractName const &contract_name, std::string const &source)
  {
    
    SynergeticContract ret = SynergeticContractClass::New(compiler_, contract_name, source);
    contracts_[contract_name] = ret;
    return ret;
  }

  SynergeticContract GetContract(ContractName const &name)
  {
    auto it = contracts_.find(name);
    if(it == contracts_.end())
    {
      return nullptr;
    }
    return it->second;
  }
private:
  vm::Module     module_;
  vm::Compiler  *compiler_ = nullptr;
  std::unordered_map< ContractName, SynergeticContract > contracts_;
};

}
}