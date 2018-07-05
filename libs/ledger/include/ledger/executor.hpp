#ifndef FETCH_EXECUTOR_HPP
#define FETCH_EXECUTOR_HPP

#include "ledger/chain/block.hpp"
#include "ledger/lane_resources.hpp"
#include "ledger/chaincode/factory.hpp"
#include "ledger/lane_interface.hpp"
#include "crypto/fnv.hpp"

#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace fetch {
namespace ledger {

/**
 * The executor object is designed to process incoming transactions
 */
class Executor {
public:
  using tx_digest_type = chain::Transaction::digest_type;
  using shared_tx_type = std::shared_ptr<chain::Transaction>;
  using tx_store_type = std::unordered_map<tx_digest_type, shared_tx_type, crypto::CallableFNV>;
  using block_digest_type = fetch::byte_array::ConstByteArray;
  using tx_digest_list_type = std::vector<tx_digest_type>;
  using lane_index_type = uint16_t;
  using lane_type = std::shared_ptr<LaneInterface>;
  using lane_list_type = std::vector<lane_type>;
  using lane_set_type = std::unordered_set<lane_index_type>;
  using chain_code_type = ChainCodeFactory::chain_code_type;
  using contract_cache_type = std::unordered_map<std::string, chain_code_type>;

  enum class Status {
    SUCCESS = 0,
    TX_LOOKUP_FAILURE,
    CHAIN_CODE_LOOKUP_FAILURE,
    CHAIN_CODE_EXEC_FAILURE
  };

  Status Execute(tx_digest_type const &hash, std::size_t slice, lane_set_type const &lanes);

private:

  lane_list_type lanes_;                  ///< The interfaces to the lanes of the system
  LaneResources resources_;               ///< The collection of lane resources (as presented to chain code)
  ChainCodeFactory factory_;              ///< The factory to create new chain code instances
  contract_cache_type chain_code_cache_;  ///< The cache of the chain code instances

  Executor::chain_code_type LookupChainCode(std::string const &name);
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTOR_HPP
