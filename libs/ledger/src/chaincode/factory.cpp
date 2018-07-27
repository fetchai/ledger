#include "ledger/chaincode/factory.hpp"

#include "ledger/chaincode/dummy_contract.hpp"
#include "ledger/chaincode/token_contract.hpp"

#include <stdexcept>

namespace fetch {
namespace ledger {
namespace {
using factory_registry_type = ChainCodeFactory::factory_registry_type;
using contract_set_type     = ChainCodeFactory::contract_set_type;

factory_registry_type CreateRegistry()
{
  factory_registry_type registry;

  registry["fetch.dummy"] = []() { return std::make_shared<DummyContract>(); };
  registry["fetch.token"] = []() { return std::make_shared<TokenContract>(); };

  return registry;
}

contract_set_type CreateContractSet(factory_registry_type const &registry)
{
  contract_set_type contracts;

  for (auto const &entry : registry)
  {
    contracts.insert(entry.first);
  }

  return contracts;
}

factory_registry_type const global_registry     = CreateRegistry();
contract_set_type const     global_contract_set = CreateContractSet(global_registry);

}  // namespace

ChainCodeFactory::chain_code_type ChainCodeFactory::Create(std::string const &name) const
{

  // lookup the chain code instance
  auto it = global_registry.find(name);
  if (it == global_registry.end())
  {
    throw std::runtime_error("Invalid chain code name");
  }

  // create the chain code instance
  chain_code_type chain_code = it->second();
  if (!chain_code)
  {
    throw std::runtime_error("Unable to create required chain code");
  }

  return chain_code;
}

ChainCodeFactory::contract_set_type const &ChainCodeFactory::GetContracts() const
{
  return global_contract_set;
}

}  // namespace ledger
}  // namespace fetch
