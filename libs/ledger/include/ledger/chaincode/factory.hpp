#ifndef FETCH_CHAINCODE_FACTORY_HPP
#define FETCH_CHAINCODE_FACTORY_HPP

#include "ledger/chaincode/contract.hpp"

#include <memory>
#include <string>

namespace fetch {
namespace ledger {

class ChainCodeFactory {
public:
  using chain_code_type = std::shared_ptr<Contract>;

  chain_code_type Create(std::string const &name);
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_CHAINCODE_FACTORY_HPP
