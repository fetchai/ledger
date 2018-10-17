//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "ledger/chaincode/smart_contract.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/script/variant.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chaincode/vm_definition.hpp"

#include <stdexcept>

namespace fetch {
namespace ledger {

SmartContract::SmartContract(vm::Script const &script)
  : Contract(script.name)
  , script_(script)
{
  for (auto &fnc : script.functions)
  {
    OnTransaction(fnc.name, this, &SmartContract::InvokeContract);
  }

  module_ = CreateVMDefinition(this);
  vm_     = std::make_unique<vm::VM>(module_.get());
}

Contract::Status SmartContract::InvokeContract(Transaction const &tx)
{
  Identifier identifier;
  identifier.Parse(static_cast<std::string>(tx.contract_name()));

  if (!vm_->Execute(script_, static_cast<std::string>(identifier.name())))
  {
    return Status::FAILED;
  }

  return Status::OK;
}

}  // namespace ledger
}  // namespace fetch
