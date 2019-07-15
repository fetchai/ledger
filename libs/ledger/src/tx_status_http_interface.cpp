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

#include "ledger/tx_status_http_interface.hpp"

#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "core/macros.hpp"
#include "http/json_response.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "variant/variant.hpp"

static constexpr char const *LOGGING_NAME = "TxStatusHttp";

using fetch::byte_array::FromHex;
using fetch::byte_array::ToBase64;
using fetch::variant::Variant;

namespace fetch {
namespace ledger {

namespace {
Variant toVariant(Digest const &digest, TransactionStatusCache::TxStatus const &tx_status)
{
  auto retval{Variant::Object()};
  retval["tx"]     = ToBase64(digest);
  retval["status"] = ToString(tx_status.status);
  auto contr_ex_res_v{Variant::Object()};
  contr_ex_res_v["status"]      = ToString(tx_status.contract_exec_result.status);
  contr_ex_res_v["exit_code"]   = tx_status.contract_exec_result.return_value;
  contr_ex_res_v["charge"]      = tx_status.contract_exec_result.charge;
  contr_ex_res_v["charge_rate"] = tx_status.contract_exec_result.charge_rate;
  contr_ex_res_v["fee"]         = tx_status.contract_exec_result.fee;

  retval["contract_exec_result"] = contr_ex_res_v;

  return retval;
}
}  // namespace

TxStatusHttpInterface::TxStatusHttpInterface(TransactionStatusCache &status_cache)
  : status_cache_{status_cache}
{
  Get("/api/status/tx/(digest=[a-fA-F0-9]{64})",
      [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
        FETCH_UNUSED(request);

        if (params.Has("digest"))
        {
          // convert the digest back to binary
          auto const digest = FromHex(params["digest"]);

          FETCH_LOG_DEBUG(LOGGING_NAME, "Querying status of: ", digest.ToBase64());

          // prepare the response
          auto const response{toVariant(digest, status_cache_.Query(digest))};

          return http::CreateJsonResponse(response);
        }
        else
        {
          return http::CreateJsonResponse("{}", http::Status::CLIENT_ERROR_BAD_REQUEST);
        }
      });
}

}  // namespace ledger
}  // namespace fetch
