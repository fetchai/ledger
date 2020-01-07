#pragma once
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

#include "chain/transaction_layout.hpp"
#include "core/digest.hpp"
#include "core/mutex.hpp"

#include <deque>
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
  using TxLayouts = std::vector<chain::TransactionLayout>;

  RecentTransactionsCache(std::size_t max_cache_size, uint32_t log2_num_lanes);
  ~RecentTransactionsCache() = default;

  void        Add(chain::Transaction const &tx);
  std::size_t GetSize() const;
  TxLayouts   Flush(std::size_t num_to_flush);

private:
  using LayoutQueue = std::deque<chain::TransactionLayout>;

  std::size_t const max_cache_size_;
  uint32_t const    log2_num_lanes_;

  mutable Mutex lock_;
  DigestSet     digests_;
  LayoutQueue   queue_;
};

}  // namespace ledger
}  // namespace fetch
