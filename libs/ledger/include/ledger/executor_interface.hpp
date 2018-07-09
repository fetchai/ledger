#ifndef FETCH_EXECUTOR_INTERFACE_HPP
#define FETCH_EXECUTOR_INTERFACE_HPP

#include "ledger/chain/transaction.hpp"

namespace fetch {
namespace ledger {

class ExecutorInterface {
public:
  using tx_digest_type = chain::Transaction::digest_type;
  using lane_index_type = uint16_t;
  using lane_set_type = std::unordered_set<lane_index_type>;

  enum class Status {
    SUCCESS = 0,
    TX_LOOKUP_FAILURE,
    CHAIN_CODE_LOOKUP_FAILURE,
    CHAIN_CODE_EXEC_FAILURE
  };

  /// @name Executor Interface
  /// @{
  virtual Status Execute(tx_digest_type const &hash, std::size_t slice, lane_set_type const &lanes) = 0;
  /// @}
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTOR_INTERFACE_HPP
