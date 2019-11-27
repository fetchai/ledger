//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "ledger/storage_unit/recent_transaction_cache.hpp"
#include "chain/transaction.hpp"

namespace fetch {
namespace ledger {

RecentTransactionsCache::RecentTransactionsCache(std::size_t max_cache_size,
                                                 uint32_t    log2_num_lanes)
  : max_cache_size_{max_cache_size}
  , log2_num_lanes_{log2_num_lanes}
{}

void RecentTransactionsCache::Add(chain::Transaction const &tx)
{
  cache_.ApplyVoid([&tx, this](Cache &cache) {
    if (cache.size() < max_cache_size_)
    {
      // convert and add the transaction layout to the cache
      cache.emplace(chain::TransactionLayout{tx, log2_num_lanes_});
    }
  });
}

std::size_t RecentTransactionsCache::GetSize() const
{
  return cache_.Apply([](Cache const &cache) { return cache.size(); });
}

RecentTransactionsCache::Cache RecentTransactionsCache::Flush()
{
  Cache output{};

  cache_.ApplyVoid([&output](Cache &cache) { std::swap(output, cache); });

  return output;
}

}  // namespace ledger
}  // namespace fetch