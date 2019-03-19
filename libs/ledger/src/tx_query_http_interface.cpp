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

#include "ledger/tx_query_http_interface.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/logger.hpp"
#include "core/macros.hpp"
#include "http/json_response.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "miner/resource_mapper.hpp"
#include "variant/variant.hpp"

static constexpr char const *LOGGING_NAME = "TxQueryAPI";

using fetch::byte_array::FromHex;
using fetch::byte_array::ToBase64;
using fetch::variant::Variant;

namespace fetch {
namespace ledger {

TxQueryHttpInterface::TxQueryHttpInterface(StorageUnitInterface &storage_unit,
                                           uint32_t              log2_num_lanes)
  : storage_unit_{storage_unit}
  , log2_num_lanes_{log2_num_lanes}
{
  Get("/api/tx/(digest=[a-fA-F0-9]{64})/", [this](http::ViewParameters const &params,
                                                  http::HTTPRequest const &   request) {
    FETCH_UNUSED(request);

    if (!params.Has("digest"))
    {
      return http::CreateJsonResponse("{}", http::Status::CLIENT_ERROR_BAD_REQUEST);
    }

    // convert the digest back to binary
    auto const digest = FromHex(params["digest"]);

    FETCH_LOG_DEBUG(LOGGING_NAME, "Querying tx: ", digest.ToBase64());

    // attempt to lookup the transaction
    Transaction tx;
    if (!storage_unit_.GetTransaction(digest, tx))
    {
      return http::CreateJsonResponse("{}", http::Status::CLIENT_ERROR_NOT_FOUND);
    }

    // prepare the response
    auto const &summary = tx.summary();

    Variant tx_obj         = Variant::Object();
    tx_obj["digest"]       = ToBase64(tx.digest());
    tx_obj["fee"]          = summary.fee;
    tx_obj["contractName"] = summary.contract_name;
    tx_obj["data"]         = ToBase64(tx.data());
    tx_obj["resources"]    = Variant::Array(summary.resources.size());

    std::size_t res_idx{0};
    for (auto const &resource : summary.resources)
    {
      Variant res_obj     = Variant::Object();
      res_obj["resource"] = ToBase64(resource);
      res_obj["lane"] = miner::MapResourceToLane(resource, summary.contract_name, log2_num_lanes_);

      tx_obj["resources"][res_idx++] = res_obj;
    }

    return http::CreateJsonResponse(tx_obj);
  });
}

}  // namespace ledger
}  // namespace fetch
