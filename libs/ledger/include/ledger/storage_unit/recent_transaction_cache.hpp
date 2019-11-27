#pragma once
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

#include "chain/transaction_layout.hpp"
#include "core/synchronisation/protected.hpp"

#include <unordered_set>
#include <vector>

namespace fetch {
namespace chain {

class Transaction;

}  // namespace chain

namespace ledger {

/**
 * The recent transactions cache is a fixed size of transactions that can be collected periodically
 * by the miner to be incorporated into the subsequent blocks
 */
class RecentTransactionsCache
{
public:
  using Cache = std::unordered_set<chain::TransactionLayout>;

  explicit RecentTransactionsCache(std::size_t max_cache_size, uint32_t log2_num_lanes);
  ~RecentTransactionsCache() = default;

  void        Add(chain::Transaction const &tx);
  std::size_t GetSize() const;
  Cache       Flush();

private:
  std::size_t const max_cache_size_;
  uint32_t const    log2_num_lanes_;
  Protected<Cache>  cache_;
};

}  // namespace ledger
}  // namespace fetch
