#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ledger/chaincode/contract_context_attacher.hpp"
#include "vm/module.hpp"

namespace fetch {
namespace vm_modules {
namespace ledger {

template <typename Contract>
void BindBalanceFunction(vm::Module &module, Contract const &contract)
{
  module.CreateFreeFunction("balance", [&contract](vm::VM *) -> uint64_t {
    decltype(auto) c = contract.context();

    fetch::ledger::ContractContextAttacher raii(*c.token_contract, c);
    c.state_adapter->PushContext("fetch.token");

    auto const balance = c.token_contract->GetBalance(c.contract_address);

    c.state_adapter->PopContext();

    return balance;
  });
}

}  // namespace ledger
}  // namespace vm_modules
}  // namespace fetch
