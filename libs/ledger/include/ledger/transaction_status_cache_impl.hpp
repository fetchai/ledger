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

#include "core/mutex.hpp"
#include "core/threading/synchronised_state.hpp"
#include "ledger/chain/digest.hpp"
#include "ledger/execution_result.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "network/generics/milli_timer.hpp"

#include <chrono>
#include <unordered_map>

namespace fetch {
namespace ledger {

template <typename CLOCK = std::chrono::steady_clock>
class TransactionStatusCacheImpl : public TransactionStatusCache
{
public:
  using Clock     = CLOCK;
  using Timepoint = typename Clock::time_point;

  struct TxStatusEx
  {
    TxStatus  status{};
    Timepoint timestamp{Clock::now()};
  };

  // Construction / Destruction
  TransactionStatusCacheImpl()                                   = default;
  TransactionStatusCacheImpl(TransactionStatusCacheImpl const &) = delete;
  TransactionStatusCacheImpl(TransactionStatusCacheImpl &&)      = delete;

  TxStatus Query(Digest digest) const override;
  void     Update(Digest digest, TransactionStatus status) override;
  void     Update(Digest digest, ContractExecutionResult exec_result) override;

  // Operators
  TransactionStatusCacheImpl &operator=(TransactionStatusCacheImpl const &) = delete;
  TransactionStatusCacheImpl &operator=(TransactionStatusCacheImpl &&) = delete;

private:
  using Mutex = mutex::Mutex;

  using Cache = DigestMap<TxStatusEx>;

  static constexpr std::chrono::hours   LIFETIME{24};
  static constexpr std::chrono::minutes INTERVAL{5};

  void PruneCache(Timepoint const &until);
  void PruneCacheIfNecessary(Timepoint const &until);

  mutable Mutex mtx_{__LINE__, __FILE__};
  Cache         cache_{};
  Timepoint     last_clean_{Clock::now()};
};

template <typename CLOCK>
constexpr std::chrono::hours TransactionStatusCacheImpl<CLOCK>::LIFETIME;
template <typename CLOCK>
constexpr std::chrono::minutes TransactionStatusCacheImpl<CLOCK>::INTERVAL;

template <typename CLOCK>
typename TransactionStatusCacheImpl<CLOCK>::TxStatus TransactionStatusCacheImpl<CLOCK>::Query(
    Digest digest) const
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

template <typename CLOCK>
void TransactionStatusCacheImpl<CLOCK>::Update(Digest digest, TransactionStatus status)
{
  auto const now{Clock::now()};

  FETCH_LOCK(mtx_);

  if (TransactionStatus::EXECUTED == status)
  {
    FETCH_LOG_WARN("TransactionStatusCache",
                   "Using inappropriate method to update contract "
                   "execution result, tx digest = " +
                       digest.ToBase64());

    throw std::runtime_error(
        "TransactionStatusCache::Update(...): Using inappropriate method to update"
        "contract execution result");
  }

  auto it = cache_.find(digest);

  if (it == cache_.end())
  {
    cache_.emplace(digest, TxStatusEx{TxStatus{status}, now});
  }
  else
  {
    it->second.status.status = status;
  }

  PruneCacheIfNecessary(now);
}

template <typename CLOCK>
void TransactionStatusCacheImpl<CLOCK>::Update(Digest digest, ContractExecutionResult exec_result)
{
  constexpr auto tx_workflow_status{TransactionStatus::EXECUTED};
  auto const     now{Clock::now()};

  FETCH_LOCK(mtx_);

  // update the cache
  auto it = cache_.find(digest);
  if (it == cache_.end())
  {
    FETCH_LOG_WARN("TransactionStatusCache",
                   "Updating contract execution status for transaction"
                   "which is missing in the tx statuc cache, tx digest = " +
                       digest.ToBase64());

    cache_.emplace(digest, TxStatusEx{TxStatus{tx_workflow_status, std::move(exec_result)}, now});
  }
  else
  {
    it->second.status.status               = tx_workflow_status;
    it->second.status.contract_exec_result = std::move(exec_result);
  }

  PruneCacheIfNecessary(now);
}

template <typename CLOCK>
void TransactionStatusCacheImpl<CLOCK>::PruneCache(Timepoint const &until)
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

template <typename CLOCK>
void TransactionStatusCacheImpl<CLOCK>::PruneCacheIfNecessary(Timepoint const &until)
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
