#ifndef FETCH_EXAMPLE_CHAINCODE_HPP
#define FETCH_EXAMPLE_CHAINCODE_HPP

#include "ledger/chaincode/contract.hpp"

#include <atomic>

namespace fetch {
namespace ledger {

class DummyContract : public Contract
{
public:
  using counter_type = std::atomic<std::size_t>;

  DummyContract();
  ~DummyContract() = default;

  std::size_t counter() const { return counter_; }

private:
  Status Wait(transaction_type const &tx);
  Status Run(transaction_type const &tx);

  counter_type counter_{0};
};

}  // namespace ledger
}  // namespace fetch

#endif  // FETCH_EXAMPLE_CHAINCODE_HPP
