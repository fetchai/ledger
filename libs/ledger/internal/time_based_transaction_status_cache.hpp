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

#include "core/digest.hpp"
#include "core/mutex.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "moment/clocks.hpp"

#include <chrono>
#include <unordered_map>

namespace fetch {
namespace ledger {

class TimeBasedTransactionStatusCache : public TransactionStatusInterface
{
public:
  using Timestamp = moment::ClockInterface::Timestamp;

  // Construction / Destruction
  TimeBasedTransactionStatusCache()                                        = default;
  TimeBasedTransactionStatusCache(TimeBasedTransactionStatusCache const &) = delete;
  TimeBasedTransactionStatusCache(TimeBasedTransactionStatusCache &&)      = delete;

  /// @name Transaction Status Interface
  /// @{
  TxStatus Query(Digest digest) const override;
  void     Update(Digest digest, TransactionStatus status) override;
  void     Update(Digest digest, ContractExecutionResult exec_result) override;
  /// @}

  // Operators
  TimeBasedTransactionStatusCache &operator=(TimeBasedTransactionStatusCache const &) = delete;
  TimeBasedTransactionStatusCache &operator=(TimeBasedTransactionStatusCache &&) = delete;

private:
  struct CacheEntry
  {
    TxStatus  status{};
    Timestamp timestamp{};
  };

  using Cache    = DigestMap<CacheEntry>;
  using ClockPtr = moment::ClockPtr;

  void PruneCache(Timestamp const &until);
  void PruneCacheIfNecessary(Timestamp const &until);

  mutable Mutex mtx_;
  ClockPtr      clock_{moment::GetClock("tx-status")};
  Cache         cache_{};
  Timestamp     last_clean_{clock_->Now()};
};

}  // namespace ledger
}  // namespace fetch
