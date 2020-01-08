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

#include "crypto/fnv.hpp"  // needed for std::hash<ConstByteArray>
#include "ledger/chaincode/contract.hpp"
#include "vm_modules/ledger/context.hpp"

#include <memory>
#include <string>

namespace fetch {

namespace vm {
struct Executable;
class Module;
}  // namespace vm

namespace chain {

class Address;

}  // namespace chain

namespace ledger {

/**
 * Smart Contract instance
 */
class SmartContract : public Contract
{
public:
  using ConstByteArray = byte_array::ConstByteArray;
  using Executable     = fetch::vm::Executable;
  using ExecutablePtr  = std::shared_ptr<Executable>;

  // Construction / Destruction
  explicit SmartContract(std::string const &source);
  ~SmartContract() override = default;

  ConstByteArray contract_digest() const
  {
    return digest_;
  }

  ExecutablePtr executable()
  {
    return executable_;
  }

private:
  using ModulePtr = std::shared_ptr<vm::Module>;

  // Transaction /
  Result InvokeAction(std::string const &name, chain::Transaction const &tx);
  Status InvokeQuery(std::string const &name, Query const &request, Query &response);
  Result InvokeInit(chain::Address const &owner, chain::Transaction const &tx);

  std::string                    source_;      ///< The source of the current contract
  ConstByteArray                 digest_;      ///< The digest of the current contract
  ExecutablePtr                  executable_;  ///< The internal script object of the parsed source
  ModulePtr                      module_;      ///< The internal module instance for the contract
  std::string                    init_fn_name_;
  vm_modules::ledger::ContextPtr context_;
};

}  // namespace ledger
}  // namespace fetch
