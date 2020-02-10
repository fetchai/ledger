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
#include "ledger/execution_result.hpp"

namespace fetch {
namespace ledger {

enum class TransactionStatus : uint8_t
{
  UNKNOWN = 0,  ///< The status of the transaction is unknown
  PENDING,      ///< The transaction is waiting to be mined
  MINED,        ///< The transaction has been mined
  EXECUTED,     ///< The transaction has been executed
  SUBMITTED,    ///< Special case for the data based synergetic transactions
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

class TransactionStatusInterface;

using TransactionStatusPtr = std::shared_ptr<TransactionStatusInterface>;

class TransactionStatusInterface
{
public:
  struct TxStatus
  {
    TransactionStatus       status{TransactionStatus::UNKNOWN};
    ContractExecutionResult contract_exec_result{};
  };

  // Factory Methods
  static TransactionStatusPtr CreateTimeBasedCache();
  static TransactionStatusPtr CreatePersistentCache();

  // Construction / Destruction
  TransactionStatusInterface()          = default;
  virtual ~TransactionStatusInterface() = default;

  /// @name Transaction Status Interface
  /// @{

  /**
   * Query the status of a specified transaction
   *
   * @param digest The digest of the transaction
   * @return The status object associated for this transaction
   */
  virtual TxStatus Query(Digest digest) const = 0;

  /**
   * Update the status of a transaction with the specified status enum
   *
   * @param digest The transaction to be updated
   * @param status The status value that should be set
   */
  virtual void Update(Digest digest, TransactionStatus status) = 0;

  /**
   * Update the contract execution result for the specified transaction
   *
   * @param digest The transaction to be updated
   * @param exec_result The contract execution result
   */
  virtual void Update(Digest digest, ContractExecutionResult exec_result) = 0;
  /// @}
};

}  // namespace ledger
}  // namespace fetch
