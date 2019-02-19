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

#include "ledger/chaincode/factory.hpp"
#include "core/logger.hpp"
#include "ledger/chaincode/dummy_contract.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/chaincode/token_contract.hpp"

#include <stdexcept>

static constexpr char const *LOGGING_NAME = "ChainCodeFactory";

namespace fetch {
namespace ledger {
namespace {

using FactoryRegistry = ChainCodeFactory::FactoryRegistry;
using ContractNameSet = ChainCodeFactory::ContractNameSet;

FactoryRegistry CreateRegistry()
{
  FactoryRegistry registry;

  registry["fetch.dummy"]                  = []() { return std::make_shared<DummyContract>(); };
  registry["fetch.token"]                  = []() { return std::make_shared<TokenContract>(); };
  registry["fetch.smart_contract_manager"] = []() {
    return std::make_shared<SmartContractManager>();
  };

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

FactoryRegistry       global_registry     = CreateRegistry();
ContractNameSet const global_contract_set = CreateContractSet(global_registry);

}  // namespace

ChainCodeFactory::ContractPtr ChainCodeFactory::Create(ContractName const &name) const
{

  // lookup the chain code instance
  auto it = global_registry.find(name);
  if (it == global_registry.end())
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Unable to lookup requested chain code: ", name,
                   ". Creating new SC");

    return std::make_shared<SmartContract>(name);
  }

  // create the chain code instance
  ContractPtr chain_code = it->second();
  if (!chain_code)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Unable to construct requested chain code: ", name);
    throw std::runtime_error("Unable to create required chain code");
  }

  return chain_code;
}

ChainCodeFactory::ContractNameSet const &ChainCodeFactory::GetContracts() const
{
  return global_contract_set;
}

}  // namespace ledger
}  // namespace fetch
