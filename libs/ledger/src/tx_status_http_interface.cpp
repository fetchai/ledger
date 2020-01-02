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

#include "core/byte_array/decoders.hpp"
#include "core/macros.hpp"
#include "http/json_response.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "ledger/tx_status_http_interface.hpp"
#include "logging/logging.hpp"
#include "variant/variant.hpp"

#include <utility>

namespace fetch {
namespace ledger {

namespace {

constexpr char const *LOGGING_NAME = "TxStatusHttp";

using fetch::byte_array::FromHex;
using fetch::variant::Variant;

constexpr PublicTxStatus Convert(TransactionStatus       tx_processing_pipeline_status,
                                 ContractExecutionStatus contract_exec_status)
{
  switch (tx_processing_pipeline_status)
  {
  case TransactionStatus::UNKNOWN:
    break;

  case TransactionStatus::PENDING:
    return PublicTxStatus::PENDING;

  case TransactionStatus::MINED:
    return PublicTxStatus::MINED;

  case TransactionStatus::EXECUTED:
    switch (contract_exec_status)
    {
    case ContractExecutionStatus::SUCCESS:
      return PublicTxStatus::EXECUTED;

    case ContractExecutionStatus::INSUFFICIENT_AVAILABLE_FUNDS:
      return PublicTxStatus::INSUFFICIENT_AVAILABLE_FUNDS;

    case ContractExecutionStatus::CONTRACT_NAME_PARSE_FAILURE:
      return PublicTxStatus::CONTRACT_NAME_PARSE_FAILURE;

    case ContractExecutionStatus::CONTRACT_LOOKUP_FAILURE:
      return PublicTxStatus::CONTRACT_LOOKUP_FAILURE;

    case ContractExecutionStatus::ACTION_LOOKUP_FAILURE:
      return PublicTxStatus::ACTION_LOOKUP_FAILURE;

    case ContractExecutionStatus::CONTRACT_EXECUTION_FAILURE:
      return PublicTxStatus::CONTRACT_EXECUTION_FAILURE;

    case ContractExecutionStatus::TRANSFER_FAILURE:
      return PublicTxStatus::TRANSFER_FAILURE;

    case ContractExecutionStatus::INSUFFICIENT_CHARGE:
      return PublicTxStatus::INSUFFICIENT_CHARGE;

    case ContractExecutionStatus::TX_LOOKUP_FAILURE:
    case ContractExecutionStatus::TX_NOT_VALID_FOR_BLOCK:
    case ContractExecutionStatus::TX_PERMISSION_DENIED:
    case ContractExecutionStatus::TX_NOT_ENOUGH_CHARGE:
    case ContractExecutionStatus::TX_CHARGE_LIMIT_TOO_HIGH:
    case ContractExecutionStatus::NOT_RUN:
    case ContractExecutionStatus::INTERNAL_ERROR:
    case ContractExecutionStatus::INEXPLICABLE_FAILURE:
      break;
    }
    return PublicTxStatus::FATAL_ERROR;

  case TransactionStatus::SUBMITTED:
    return PublicTxStatus::SUBMITTED;
  }

  return PublicTxStatus::UNKNOWN;
}

Variant ToVariant(Digest const &digest, TransactionStatusCache::TxStatus const &tx_status)
{
  auto retval{Variant::Object()};

  retval["tx"]        = digest.ToHex();
  retval["status"]    = ToString(Convert(tx_status.status, tx_status.contract_exec_result.status));
  retval["exit_code"] = tx_status.contract_exec_result.return_value;
  retval["charge"]    = tx_status.contract_exec_result.charge;
  retval["charge_rate"] = tx_status.contract_exec_result.charge_rate;
  retval["fee"]         = tx_status.contract_exec_result.fee;

  return retval;
}
}  // namespace

TxStatusHttpInterface::TxStatusHttpInterface(TxStatusCachePtr status_cache)
  : status_cache_{std::move(status_cache)}
{
  assert(status_cache_);

  Get("/api/status/tx/(digest=[a-fA-F0-9]{64})", "Retrieves a transaction status.",
      {
          {"digest", "The transaction hash.", http::validators::StringValue()},
      },
      [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
        FETCH_UNUSED(request);

        if (params.Has("digest"))
        {
          auto const digest = FromHex(params["digest"]);

          FETCH_LOG_DEBUG(LOGGING_NAME, "Querying status of: 0x", digest.ToHex());

          auto const response{ToVariant(digest, status_cache_->Query(digest))};

          return http::CreateJsonResponse(response);
        }

        return http::CreateJsonResponse("{}", http::Status::CLIENT_ERROR_BAD_REQUEST);
      });
}

}  // namespace ledger
}  // namespace fetch
