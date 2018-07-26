#ifndef FETCH_WALLET_HPP
#define FETCH_WALLET_HPP

#include "ledger/chaincode/contract.hpp"

namespace fetch {
namespace ledger {

class TokenContract : public Contract {
public:

  TokenContract();
  ~TokenContract() = default;

private:

  // transaction handlers
  Status CreateWealth(transaction_type const &tx);
  Status Transfer(transaction_type const &tx);

  // queries
  Status Balance(query_type const &query, query_type &response);

};

} // namespace ledger
} // namespace fetch

#endif //FETCH_WALLET_HPP
