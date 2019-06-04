#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "ledger/upow/synergetic_vm_module.hpp"

#include "vm/analyser.hpp"
#include "vm/common.hpp"

#include "vm/compiler.hpp"
#include "vm/module.hpp"
#include "vm/vm.hpp"

#include "synergetic_contract.hpp"

#include <string>
#include <vector>

namespace fetch {
namespace consensus {

#if 0
class SynergeticContractRegister
{
public:
  using ContractName   = byte_array::ConstByteArray;
  using UniqueCompiler = std::unique_ptr<vm::Compiler>;

  SynergeticContractRegister()
  {
    CreateConensusVMModule(module_);
    compiler_.reset(new fetch::vm::Compiler(&module_));
  }

  ~SynergeticContractRegister() = default;

  SynergeticContract CreateContract(ContractName const &contract_name, std::string const &source)
  {
    errors_.clear();
    SynergeticContract ret =
        SynergeticContractClass::New(compiler_, contract_name, source, errors_);
    contracts_[contract_name] = ret;
    return ret;
  }

  bool HasContract(ContractName const &name) const
  {
    auto it = contracts_.find(name);
    return (it != contracts_.end());
  }

  SynergeticContract GetContract(ContractName const &name)
  {
    auto it = contracts_.find(name);
    if (it == contracts_.end())
    {
      return nullptr;
    }
    return it->second;
  }

  void Clear()
  {
    contracts_.clear();
    errors_.clear();
  }

  std::vector<std::string> const &errors() const
  {
    return errors_;
  }

private:
  vm::Module                                           module_;
  UniqueCompiler                                       compiler_{nullptr};
  std::vector<std::string>                             errors_;
  std::unordered_map<ContractName, SynergeticContract> contracts_;
};
#endif

}  // namespace consensus
}  // namespace fetch
