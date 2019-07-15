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

#include <chrono>
#include <unordered_map>

namespace fetch {
namespace ledger {

enum class eTransactionStatus : uint8_t
{
  UNKNOWN,    ///< The status of the transaction is unknown
  PENDING,    ///< The transaction is waiting to be mined
  MINED,      ///< The transaction has been mined
  EXECUTED,   ///< The transaction has been executed
  SUBMITTED,  ///< Special case for the data based synergetic transactions
};

char const *ToString(eTransactionStatus status);

class TransactionStatusCache
{
public:
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;

  struct TxStatus
  {
    eTransactionStatus      status{eTransactionStatus::UNKNOWN};
    Timepoint               timestamp{Clock::now()};
    ContractExecutionResult contract_exec_result{};
  };

  // Construction / Destruction
  TransactionStatusCache()                               = default;
  TransactionStatusCache(TransactionStatusCache const &) = delete;
  TransactionStatusCache(TransactionStatusCache &&)      = delete;
  ~TransactionStatusCache()                              = default;

  TxStatus Query(Digest digest) const;
  void Update(Digest digest, eTransactionStatus status, ContractExecutionResult const execRes = {},
              Timepoint const &now = Clock::now());

  // Operators
  TransactionStatusCache &operator=(TransactionStatusCache const &) = delete;
  TransactionStatusCache &operator=(TransactionStatusCache &&) = delete;

private:
  using Mutex = mutex::Mutex;

  using Cache = DigestMap<TxStatus>;

  void PruneCache(Timepoint const &now);

  mutable Mutex mtx_{__LINE__, __FILE__};
  Cache         cache_{};
  Timepoint     last_clean_{Clock::now()};
};

}  // namespace ledger
}  // namespace fetch
