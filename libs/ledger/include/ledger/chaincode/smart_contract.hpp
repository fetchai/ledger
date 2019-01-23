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

#include "ledger/chaincode/contract.hpp"  // for Contract, Contract::Status
#include "ledger/chaincode/vm_definition.hpp"
#include "vm/defs.hpp"                    // for Script
#include "vm/module.hpp"
#include "vm/vm.hpp"

namespace fetch {
namespace ledger {

class SmartContract : public Contract
{
public:
  SmartContract(vm::Script const &script);
  ~SmartContract() = default;

private:
  Status InvokeContract(Transaction const &tx);

  vm::Script                  script_;
  std::unique_ptr<vm::Module> module_;
  std::unique_ptr<vm::VM>     vm_;
};

}  // namespace ledger
}  // namespace fetch
