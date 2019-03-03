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

#include "core/macros.hpp"
#include "core/logger.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/byte_array/decoders.hpp"
#include "variant/variant.hpp"
#include "http/json_response.hpp"
#include "ledger/transaction_status_cache.hpp"
#include "ledger/tx_status_http_interface.hpp"

static constexpr char const *LOGGING_NAME = "TxStatusHttp";

using fetch::byte_array::FromHex;
using fetch::byte_array::ToBase64;
using fetch::variant::Variant;

namespace fetch {
namespace ledger {

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

        FETCH_LOG_INFO(LOGGING_NAME, "Querying status of: ", digest.ToBase64());

        // prepare the response
        Variant response = Variant::Object();
        response["tx"] = ToBase64(digest);
        response["status"] = ToString(status_cache_.Query(digest));

        return http::CreateJsonResponse(response);
      }
      else
      {
        return http::CreateJsonResponse("{}", http::Status::CLIENT_ERROR_BAD_REQUEST);
      }
  });
}

} // namespace ledger
} // namespace fetch
