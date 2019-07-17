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
  ~TxStatusHttpInterface()                             = default;

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
  MINED,        ///< The transaction has been mined
  SUBMITTED,    ///< Special case for the data based synergetic transactions

  // Contract Execution Errors
  SUCCESSFULLY_EXECUTED,
  CHAIN_CODE_LOOKUP_FAILURE,
  CHAIN_CODE_EXEC_FAILURE,
  CONTRACT_NAME_PARSE_FAILURE,
  CONTRACT_LOOKUP_FAILURE,
  TX_NOT_VALID_FOR_BLOCK,
  INSUFFICIENT_AVAILABLE_FUNDS,
  TRANSFER_FAILURE,
  INSUFFICIENT_CHARGE,
  FATAL_ERROR,
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
  case PublicTxStatus::SUCCESSFULLY_EXECUTED:
    return "Successfully Executed";
  case PublicTxStatus::CHAIN_CODE_LOOKUP_FAILURE:
    return "Chain Code Lookup Failure";
  case PublicTxStatus::CHAIN_CODE_EXEC_FAILURE:
    return "Chain Code Execution Failure";
  case PublicTxStatus::CONTRACT_NAME_PARSE_FAILURE:
    return "Contract Name Parse Failure";
  case PublicTxStatus::CONTRACT_LOOKUP_FAILURE:
    return "Contract Lookup Failure";
  case PublicTxStatus::TX_NOT_VALID_FOR_BLOCK:
    return "Tx not valid for current block";
  case PublicTxStatus::INSUFFICIENT_AVAILABLE_FUNDS:
    return "Insufficient available funds";
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
