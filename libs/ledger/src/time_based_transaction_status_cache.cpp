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

#include "network/generics/milli_timer.hpp"
#include "time_based_transaction_status_cache.hpp"

namespace fetch {
namespace ledger {
namespace {

constexpr std::chrono::hours   LIFETIME{24};
constexpr std::chrono::minutes INTERVAL{5};

using TxStatus = TimeBasedTransactionStatusCache::TxStatus;

}  // namespace

/**
 * Query the status of a specified transaction
 *
 * @param digest The digest of the transaction
 * @return The status object associated for this transaction
 */
TxStatus TimeBasedTransactionStatusCache::Query(Digest digest) const
{
  {
    FETCH_LOCK(mtx_);

    auto const it = cache_.find(digest);
    if (cache_.end() != it)
    {
      return it->second.status;
    }
  }

  return {};
}

/**
 * Update the status of a transaction with the specified status enum
 *
 * @param digest The transaction to be updated
 * @param status The status value that should be set
 */
void TimeBasedTransactionStatusCache::Update(Digest digest, TransactionStatus status)
{
  auto const now{clock_->Now()};

  FETCH_LOCK(mtx_);

  if (TransactionStatus::EXECUTED == status)
  {
    FETCH_LOG_WARN("TransactionStatusCache",
                   "Using inappropriate method to update contract "
                   "execution result. (tx digest: 0x",
                   digest.ToHex(), ")");

    throw std::runtime_error(
        "TransactionStatusCache::Update(...): Using inappropriate method to update"
        "contract execution result");
  }

  auto it = cache_.find(digest);

  if (it == cache_.end())
  {
    cache_.emplace(digest, CacheEntry{TxStatus{status}, now});
  }
  else
  {
    it->second.status.status = status;
  }

  PruneCacheIfNecessary(now);
}

/**
 * Update the contract execution result for the specified transaction
 *
 * @param digest The transaction to be updated
 * @param exec_result The contract execution result
 */
void TimeBasedTransactionStatusCache::Update(Digest digest, ContractExecutionResult exec_result)
{
  static constexpr TransactionStatus EXECUTED_STATUS{TransactionStatus::EXECUTED};

  auto const now{clock_->Now()};

  FETCH_LOCK(mtx_);

  // update the cache
  auto it = cache_.find(digest);
  if (it == cache_.end())
  {
    FETCH_LOG_DEBUG("TransactionStatusCache",
                    "Updating contract execution status for transaction "
                    "which is missing in the tx status cache. (tx digest: 0x",
                    digest.ToHex(), ")");

    cache_.emplace(digest, CacheEntry{TxStatus{EXECUTED_STATUS, exec_result}, now});
  }
  else
  {
    it->second.status.status               = EXECUTED_STATUS;
    it->second.status.contract_exec_result = exec_result;
  }

  PruneCacheIfNecessary(now);
}

void TimeBasedTransactionStatusCache::PruneCache(Timestamp const &until)
{
  fetch::generics::MilliTimer timer{"TxStatusCache::Prune"};

  auto it = cache_.begin();
  while (it != cache_.end())
  {
    auto const age = until - it->second.timestamp;

    if (age > LIFETIME)
    {
      it = cache_.erase(it);
    }
    else
    {
      ++it;
    }
  }

  last_clean_ = until;
}

void TimeBasedTransactionStatusCache::PruneCacheIfNecessary(Timestamp const &until)
{
  auto const delta_prune = until - last_clean_;
  if (delta_prune < INTERVAL)
  {
    return;
  }

  PruneCache(until);
}

}  // namespace ledger
}  // namespace fetch
