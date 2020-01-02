//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "chain/transaction.hpp"
#include "core/containers/is_in.hpp"
#include "ledger/storage_unit/recent_transaction_cache.hpp"

using fetch::core::IsIn;

namespace fetch {
namespace ledger {

/**
 * Build a recent transactions cache
 *
 * @param max_cache_size The maximum size of the cache in entries
 * @param log2_num_lanes The log2 of the number of lanes in the system
 */
RecentTransactionsCache::RecentTransactionsCache(std::size_t max_cache_size,
                                                 uint32_t    log2_num_lanes)
  : max_cache_size_{max_cache_size}
  , log2_num_lanes_{log2_num_lanes}
{}

/**
 * Add a recent transaction to the cache
 *
 * @param tx The transaction to be added to the queue
 */
void RecentTransactionsCache::Add(chain::Transaction const &tx)
{
  FETCH_LOCK(lock_);

  // if we haven't seen this before then add it to the cache
  if (!IsIn(digests_, tx.digest()))
  {
    digests_.emplace(tx.digest());
    queue_.emplace_front(chain::TransactionLayout{tx, log2_num_lanes_});
  }

  // if we have reached the capacity of the cache start dropping the oldest ones
  while (queue_.size() > max_cache_size_)
  {
    digests_.erase(queue_.back().digest());
    queue_.pop_back();
  }
}

/**
 * Get the size of the cache in entries
 *
 * @return The number of tx layouts stored in the cache
 */
std::size_t RecentTransactionsCache::GetSize() const
{
  FETCH_LOCK(lock_);
  return queue_.size();
}

/**
 * Flush the N most recent layout entries in the cache
 *
 * @param num_to_flush The maximum number of entries to extract from the cache
 * @return The layouts extracted from the cache
 */
RecentTransactionsCache::TxLayouts RecentTransactionsCache::Flush(std::size_t num_to_flush)
{
  // reserve the memory ahead of time
  TxLayouts layouts{};
  layouts.reserve(num_to_flush);

  // start popping the transaction off in most recent first order
  FETCH_LOCK(lock_);
  for (std::size_t i = 0; !queue_.empty() && i < num_to_flush; ++i)
  {
    auto &current = queue_.front();

    digests_.erase(current.digest());
    layouts.emplace_back(current);
    queue_.pop_front();
  }

  return layouts;
}

}  // namespace ledger
}  // namespace fetch
