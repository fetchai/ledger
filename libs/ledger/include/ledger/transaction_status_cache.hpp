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
#include "network/generics/milli_timer.hpp"

#include <chrono>
#include <unordered_map>

namespace fetch {
namespace ledger {

enum class TransactionStatus : uint8_t
{
  UNKNOWN,    ///< The status of the transaction is unknown
  PENDING,    ///< The transaction is waiting to be mined
  MINED,      ///< The transaction has been mined
  EXECUTED,   ///< The transaction has been executed
  SUBMITTED,  ///< Special case for the data based synergetic transactions
};

constexpr char const *ToString(TransactionStatus status)
{
  switch (status)
  {
  case TransactionStatus::UNKNOWN:
    break;

  case TransactionStatus::PENDING:
    return "Pending";

  case TransactionStatus::MINED:
    return "Mined";

  case TransactionStatus::EXECUTED:
    return "Executed";

  case TransactionStatus::SUBMITTED:
    return "Submitted";
  }

  return "Unknown";
}

class TransactionStatusCache
{
public:
  using ShrdPtr = std::shared_ptr<TransactionStatusCache>;

  struct TxStatus
  {
    TransactionStatus       status{TransactionStatus::UNKNOWN};
    ContractExecutionResult contract_exec_result{};
  };

  virtual ~TransactionStatusCache() = default;

  virtual TxStatus Query(Digest digest) const                                 = 0;
  virtual void     Update(Digest digest, TransactionStatus status)            = 0;
  virtual void     Update(Digest digest, ContractExecutionResult exec_result) = 0;

  // Operators
  TransactionStatusCache &operator=(TransactionStatusCache const &) = delete;
  TransactionStatusCache &operator=(TransactionStatusCache &&) = delete;

  static ShrdPtr factory();
};

}  // namespace ledger
}  // namespace fetch
