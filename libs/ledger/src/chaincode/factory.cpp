#include "ledger/chaincode/factory.hpp"

#include "ledger/chaincode/dummy_contract.hpp"
#include "ledger/chaincode/token_contract.hpp"

#include <stdexcept>

namespace fetch {
namespace ledger {

ChainCodeFactory::chain_code_type ChainCodeFactory::Create(std::string const &name)
{
  chain_code_type chaincode;
  if (name == "fetch.dummy") {
    chaincode = std::make_shared<DummyContract>();
  } else if (name == "fetch.token") {
    chaincode = std::make_shared<TokenContract>();
  }

  if (!chaincode) {
    throw std::runtime_error("Unable to create required chain code");
  }

  return chaincode;
}

} // namespace ledger
} // namespace fetch
