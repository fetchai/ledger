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

#include "crypto/fnv.hpp" // needed for std::hash<ConstByteArray> !!!
#include "ledger/chaincode/contract.hpp"

#include <string>
#include <memory>

namespace fetch {

namespace vm {
  struct Script;
  class Module;
}

namespace ledger {

/**
 * Smart Contract instance.
 *
 * Contains an instance of the virtual machine
 */
class SmartContract : public Contract
{
public:
  using ConstByteArray = byte_array::ConstByteArray;

  static constexpr char const *LOGGING_NAME = "SmartContract";

  // Construction / Destruction
  explicit SmartContract(std::string const &source);
  ~SmartContract() override = default;

  ConstByteArray contract_digest() const { return digest_; }

private:
  using Script    = fetch::vm::Script;
  using ScriptPtr = std::shared_ptr<Script>;
  using ModulePtr = std::shared_ptr<vm::Module>;

  // Transaction /
  Status InvokeAction(std::string const &name, Transaction const &tx);
  Status InvokeQuery(std::string const &name, Query const &request, Query &response);

  std::string    source_; ///< The source of the current contract
  ConstByteArray digest_; ///< The digest of the current contract
  ScriptPtr      script_; ///< The internal script object of the parsed source
  ModulePtr      module_; ///< The internal module instance for the contract
};

}  // namespace ledger
}  // namespace fetch
