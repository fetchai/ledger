#ifndef FETCH_LANE_INTERFACE_HPP
#define FETCH_LANE_INTERFACE_HPP

#include "ledger/chain/transaction.hpp"

#include "core/byte_array/referenced_byte_array.hpp"

namespace fetch {
namespace ledger {

class LaneInterface {
public:
  using transaction_type = chain::Transaction;
  using transaction_hash_type = transaction_type::digest_type;

  /// @name Transaction Store Interface
  /// @{
  virtual bool HasTransaction(transaction_hash_type const &hash) = 0;
  virtual bool GetTransaction(transaction_hash_type const &hash, transaction_type &tx) = 0;
  /// @}
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_LANE_INTERFACE_HPP
