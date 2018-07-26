#ifndef FETCH_EXECUTOR_HPP
#define FETCH_EXECUTOR_HPP

#include "ledger/chain/block.hpp"
#include "ledger/chaincode/cache.hpp"
#include "ledger/executor_interface.hpp"
#include "crypto/fnv.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <vector>
#include <unordered_set>

namespace fetch {
namespace ledger {

/**
 * The executor object is designed to process incoming transactions
 */
class Executor : public ExecutorInterface {
public:
  using shared_tx_type = std::shared_ptr<chain::Transaction>;
  using tx_store_type = std::unordered_map<tx_digest_type, shared_tx_type, crypto::CallableFNV>;
  using block_digest_type = fetch::byte_array::ConstByteArray;
  using tx_digest_list_type = std::vector<tx_digest_type>;
  using resources_type = std::shared_ptr<StorageUnitInterface>;
  using chain_code_type = ChainCodeFactory::chain_code_type;
  using contract_cache_type = std::unordered_map<std::string, chain_code_type>;

  // Construction / Destruction
  explicit Executor(resources_type resources)
    : resources_{std::move(resources)} {
  }
  ~Executor() override = default;

  /// @name Executor Interface
  /// @{
  Status Execute(tx_digest_type const &hash, std::size_t slice, lane_set_type const &lanes) override;
  /// @}

private:

  resources_type resources_;            ///< The collection of resources as published by the collection of lanes
  ChainCodeCache chain_code_cache_;     ///< The factory to create new chain code instances
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_EXECUTOR_HPP
