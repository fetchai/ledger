#ifndef FETCH_CHAINCODE_FACTORY_HPP
#define FETCH_CHAINCODE_FACTORY_HPP

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
  using factory_registry_type =
      std::unordered_map<std::string, factory_callable_type>;
  using contract_set_type = std::unordered_set<std::string>;

  chain_code_type          Create(std::string const &name) const;
  contract_set_type const &GetContracts() const;
};

}  // namespace ledger
}  // namespace fetch

#endif  // FETCH_CHAINCODE_FACTORY_HPP
