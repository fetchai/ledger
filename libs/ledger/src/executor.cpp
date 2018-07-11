#include "core/byte_array/encoders.hpp"
#include "ledger/executor.hpp"

#include "core/assert.hpp"

#include "core/logger.hpp"
#include "core/mutex.hpp"

#include <chrono>
#include <thread>
#include <random>
#include <algorithm>

namespace fetch {
namespace ledger {

#if 0
std::ostream &operator<<(std::ostream &stream, Executor::lane_set_type const &lane_set)
{
  std::vector<uint16_t> elements(lane_set.size());
  std::copy(lane_set.begin(), lane_set.end(), elements.begin());
  std::sort(elements.begin(), elements.end());

  bool not_first_loop = false;
  for (auto element : elements)
  {
    if (not_first_loop)
      stream << ',';
    stream << element;
    not_first_loop = true;
  }
  return stream;
}
#endif

/**
 * Executes a given transaction across a series of lanes
 *
 * @param hash The transaction hash
 * @param slice The current block slice
 * @param lanes The affected lanes for the transaction
 * @return The status code for the operation
 */
Executor::Status Executor::Execute(tx_digest_type const &hash, std::size_t slice, lane_set_type const &lanes) {

  // Get the transaction from the store (we should be able to take the transaction from any of the lanes, for
  // simplicity, however, just pick the first one).
  chain::Transaction tx;
  if (!resources_->GetTransaction(hash, tx)) {
    return Status::TX_LOOKUP_FAILURE;
  }

  // Lookup the chain code associated with the transaction
  auto chain_code = LookupChainCode(tx.contract_name().name_space());
  if (!chain_code) {
    return Status::CHAIN_CODE_LOOKUP_FAILURE;
  }

  // attach the chain code to the current working context
  chain_code->Attach(*resources_);

  // Dispatch the transaction to the contract
  auto result = chain_code->DispatchTransaction(tx.contract_name().name(), tx);
  if (Contract::Status::OK != result) {
    return Status::CHAIN_CODE_EXEC_FAILURE;
  }

  // detach the chain code from the current context
  chain_code->Detach();

  return Status::SUCCESS;
}

/**
 * Creates or reuses a chain code instance based on a name
 *
 * @param name The name of the chain code contract
 * @return The chain code
 */
Executor::chain_code_type Executor::LookupChainCode(std::string const &name) {
  chain_code_type chain_code;

  auto it = chain_code_cache_.find(name);
  if (it != chain_code_cache_.end()) {
    chain_code = it->second;
  } else {
    chain_code = factory_.Create(name);
  }

  // update the cache
  if (chain_code) {
    chain_code_cache_[name] = chain_code;
  }

  return chain_code;
}

} // namespace ledger
} // namespace fetch


