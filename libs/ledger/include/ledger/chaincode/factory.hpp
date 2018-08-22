#pragma once
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

#include "ledger/chaincode/contract.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace fetch {
namespace ledger {

class ChainCodeFactory
{
public:
  using chain_code_type       = std::shared_ptr<Contract>;
  using factory_callable_type = std::function<chain_code_type()>;
  using factory_registry_type = std::unordered_map<std::string, factory_callable_type>;
  using contract_set_type     = std::unordered_set<std::string>;

  chain_code_type          Create(std::string const &name) const;
  contract_set_type const &GetContracts() const;
};

}  // namespace ledger
}  // namespace fetch
