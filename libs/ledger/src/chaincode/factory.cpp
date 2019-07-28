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

#include "core/logger.hpp"
#include "core/serializers/byte_array.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "ledger/chaincode/dummy_contract.hpp"
#include "ledger/chaincode/factory.hpp"
#include "ledger/chaincode/smart_contract.hpp"
#include "ledger/chaincode/smart_contract_manager.hpp"
#include "ledger/chaincode/token_contract.hpp"

#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

static constexpr char const *LOGGING_NAME = "ChainCodeFactory";

using fetch::byte_array::ConstByteArray;

namespace fetch {
namespace ledger {
namespace {

using ContractPtr     = ChainCodeFactory::ContractPtr;
using ContractNameSet = ChainCodeFactory::ContractNameSet;
using FactoryCallable = std::function<ContractPtr()>;
using FactoryRegistry = std::unordered_map<ConstByteArray, FactoryCallable>;

FactoryRegistry CreateRegistry()
{
  FactoryRegistry registry;

  registry[DummyContract::NAME]        = []() { return std::make_shared<DummyContract>(); };
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

ChainCodeFactory::ContractPtr ChainCodeFactory::Create(Identifier const &contract_id,
                                                       StorageInterface &storage) const
{
  ContractPtr contract{};

  // determine based on the identifier is the requested contract a VM based smart contract or is it
  // referencing a hard coded "chain code"
  if (Identifier::Type::SMART_CONTRACT == contract_id.type())
  {
    // create the resource address for the contract
    auto const resource = SmartContractManager::CreateAddressForContract(contract_id);

    // query the contents of the address
    auto const result = storage.Get(resource);

    if (!result.failed)
    {
      ConstByteArray contract_source;

      // deserialise the contract source
      serializers::MsgPackSerializer adapter{result.document};
      adapter >> contract_source;

      // attempt to construct the smart contract in question
      contract = std::make_shared<SmartContract>(std::string{contract_source});
    }
  }
  else  // invalid or chain code
  {
    // attempt to lookup the chain code instance
    auto it = global_registry.find(contract_id.full_name());
    if (it != global_registry.end())
    {
      // execute the factory to create the chain code instance
      contract = it->second();
    }
  }

  // finally throw an exception if the contract in question can not be found
  if (!contract)
  {
    FETCH_LOG_ERROR(LOGGING_NAME,
                    "Unable to construct requested chain code: ", contract_id.full_name());
    throw std::runtime_error("Unable to create required chain code");
  }

  return contract;
}

ContractNameSet const &ChainCodeFactory::GetChainCodeContracts() const
{
  return global_contract_set;
}

}  // namespace ledger
}  // namespace fetch
