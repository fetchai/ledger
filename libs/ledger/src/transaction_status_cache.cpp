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

#include "ledger/transaction_status_cache.hpp"
#include "network/generics/milli_timer.hpp"

static const std::chrono::hours   LIFETIME{24};
static const std::chrono::minutes INTERVAL{5};

using fetch::generics::MilliTimer;

namespace fetch {
namespace ledger {

char const *ToString(TransactionStatus status)
{
  char const *text = "Unknown";

  switch (status)
  {
  case TransactionStatus::UNKNOWN:
    break;
  case TransactionStatus::PENDING:
    text = "Pending";
    break;
  case TransactionStatus::MINED:
    text = "Mined";
    break;
  case TransactionStatus::EXECUTED:
    text = "Executed";
    break;
  case TransactionStatus::SUBMITTED:
    text = "Submitted";
    break;
  }

  return text;
}

TransactionStatus TransactionStatusCache::Query(Digest digest) const
{
  TransactionStatus status{TransactionStatus::UNKNOWN};

  {
    FETCH_LOCK(mtx_);

    auto const it = cache_.find(digest);
    if (cache_.end() != it)
    {
      status = it->second.status;
    }
  }

  return status;
}

void TransactionStatusCache::Update(Digest digest, TransactionStatus status, Timepoint const &now)
{
  FETCH_LOCK(mtx_);

  // update the cache
  cache_[digest] = Element{status, now};

  // determine if we need to prune the cache
  auto const delta_prune = now - last_clean_;
  if (delta_prune > INTERVAL)
  {
    PruneCache(now);

    last_clean_ = now;
  }
}

void TransactionStatusCache::PruneCache(Timepoint const &now)
{
  MilliTimer timer{"TxStatusCache::Prune"};

  auto it = cache_.begin();
  while (it != cache_.end())
  {
    auto const age = now - it->second.timestamp;

    if (age > LIFETIME)
    {
      it = cache_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

}  // namespace ledger
}  // namespace fetch
