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

#include "core/serializers/main_serializer.hpp"
#include "ledger/chaincode/chain_code_factory.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "logging/logging.hpp"

#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace fetch {
namespace ledger {

namespace {

using fetch::byte_array::ConstByteArray;

using ContractPtr     = std::shared_ptr<Contract>;
using ContractNameSet = std::unordered_set<ConstByteArray>;
using FactoryCallable = std::function<ContractPtr()>;
using FactoryRegistry = std::unordered_map<ConstByteArray, FactoryCallable>;

constexpr char const *LOGGING_NAME = "ChainCodeFactory";

FactoryRegistry CreateRegistry()
{
  FactoryRegistry registry;

  registry[TokenContract::NAME]        = []() { return std::make_shared<TokenContract>(); };
  registry[SmartContractManager::NAME] = []() { return std::make_shared<SmartContractManager>(); };

  return registry;
}

ContractNameSet CreateContractSet(FactoryRegistry const &registry)
{
  ContractNameSet contracts;

  for (auto const &entry : registry)
  {
    contracts.insert(entry.first);
  }

  return contracts;
}

FactoryRegistry const global_registry     = CreateRegistry();
ContractNameSet const global_contract_set = CreateContractSet(global_registry);

}  // namespace

ContractPtr CreateChainCode(ConstByteArray const &contract_name)
{
  auto it = global_registry.find(contract_name);
  if (it != global_registry.end())
  {
    // execute the factory to create the chain code instance
    return it->second();
  }

  FETCH_LOG_ERROR(LOGGING_NAME, "Unable to construct requested chain code: ", contract_name);

  throw std::runtime_error(std::string{"Unable to create requested chain code "} +
                           std::string(contract_name));
}

ContractNameSet const &GetChainCodeContracts()
{
  return global_contract_set;
}

}  // namespace ledger
}  // namespace fetch
