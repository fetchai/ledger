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

#include "http/module.hpp"

namespace fetch {
namespace ledger {

class TransactionStatusCache;

class TxStatusHttpInterface : public http::HTTPModule
{
public:
  using TxStatusCachePtr = std::shared_ptr<TransactionStatusCache>;

  // Construction / Destruction
  explicit TxStatusHttpInterface(TxStatusCachePtr status_cache);
  TxStatusHttpInterface(TxStatusHttpInterface const &) = delete;
  TxStatusHttpInterface(TxStatusHttpInterface &&)      = delete;
  ~TxStatusHttpInterface() override                    = default;

  // Operators
  TxStatusHttpInterface &operator=(TxStatusHttpInterface const &) = delete;
  TxStatusHttpInterface &operator=(TxStatusHttpInterface &&) = delete;

private:
  TxStatusCachePtr status_cache_;
};

enum class PublicTxStatus
{
  // Workflow status
  UNKNOWN = 0,  ///< The status of the transaction is unknown
  PENDING,      ///< The transaction is waiting to be mined
  MINED,        ///< The transaction has been mined (selected for execution)
  EXECUTED,     ///< The transaction has been executed successfully
  SUBMITTED,    ///< Special case for the data based synergetic transactions

  // Normal Contract execution errors
  INSUFFICIENT_AVAILABLE_FUNDS,
  CONTRACT_NAME_PARSE_FAILURE,
  CONTRACT_LOOKUP_FAILURE,
  ACTION_LOOKUP_FAILURE,
  CONTRACT_EXECUTION_FAILURE,
  TRANSFER_FAILURE,
  INSUFFICIENT_CHARGE,

  FATAL_ERROR
};

constexpr char const *ToString(PublicTxStatus status)
{
  switch (status)
  {
  case PublicTxStatus::UNKNOWN:
    return "Unknown";
  case PublicTxStatus::PENDING:
    return "Pending";
  case PublicTxStatus::MINED:
    return "Mined";
  case PublicTxStatus::SUBMITTED:
    return "Submitted";
  case PublicTxStatus::EXECUTED:
    return "Executed";
  case PublicTxStatus::INSUFFICIENT_AVAILABLE_FUNDS:
    return "Insufficient available funds";
  case PublicTxStatus::CONTRACT_NAME_PARSE_FAILURE:
    return "Contract Name Parse Failure";
  case PublicTxStatus::CONTRACT_LOOKUP_FAILURE:
    return "Contract Lookup Failure";
  case PublicTxStatus::ACTION_LOOKUP_FAILURE:
    return "Contract Action Lookup Failure";
  case PublicTxStatus::CONTRACT_EXECUTION_FAILURE:
    return "Contract Execution Failure";
  case PublicTxStatus::TRANSFER_FAILURE:
    return "Unable to perform transfer";
  case PublicTxStatus::INSUFFICIENT_CHARGE:
    return "Insufficient charge";
  case PublicTxStatus::FATAL_ERROR:
    return "Fatal Error";
  }
  return "Unknown";
}

}  // namespace ledger
}  // namespace fetch
