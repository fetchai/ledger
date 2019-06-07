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
#include "ledger/chain/transaction.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "miner/resource_mapper.hpp"
#include "variant/variant.hpp"

static constexpr char const *LOGGING_NAME = "TxQueryAPI";

using fetch::byte_array::FromHex;
using fetch::variant::Variant;

namespace fetch {
namespace ledger {

TxQueryHttpInterface::TxQueryHttpInterface(StorageUnitInterface &storage_unit)
  : storage_unit_{storage_unit}
{
  Get("/api/tx/(digest=[a-fA-F0-9]{64})/",
      [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
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

        Variant tx_obj   = Variant::Object();
        tx_obj["digest"] = "0x" + tx.digest().ToHex();
        tx_obj["from"]   = tx.from().display();

        auto const &transfers     = tx.transfers();
        auto &      transfers_arr = tx_obj["transfers"] = Variant::Array(transfers.size());

        for (std::size_t i = 0; i < transfers.size(); ++i)
        {
          auto &transfer_obj = transfers_arr[i] = Variant::Object();

          transfer_obj["to"]     = transfers[i].to.display();
          transfer_obj["amount"] = transfers[i].amount;
        }

        tx_obj["validFrom"]  = tx.valid_from();
        tx_obj["validUntil"] = tx.valid_until();

        tx_obj["charge"]      = tx.charge();
        tx_obj["chargeLimit"] = tx.charge_limit();

        switch (tx.contract_mode())
        {
        case Transaction::ContractMode::NOT_PRESENT:
          break;
        case Transaction::ContractMode::PRESENT:
          tx_obj["contractDigest"]  = tx.contract_digest().display();
          tx_obj["contractAddress"] = tx.contract_address().display();
          tx_obj["action"]          = tx.action();
          tx_obj["data"]            = tx.data().ToBase64();
          break;
        case Transaction::ContractMode::CHAIN_CODE:
          tx_obj["chainCode"] = tx.chain_code();
          tx_obj["action"]    = tx.action();
          tx_obj["data"]      = tx.data().ToBase64();
          break;
        }

        auto const &signatories     = tx.signatories();
        auto &      signatories_arr = tx_obj["signatories"] = Variant::Array(signatories.size());

        for (std::size_t i = 0; i < signatories.size(); ++i)
        {
          signatories_arr[i] = "0x" + signatories[i].identity.identifier().ToHex();
        }

        return http::CreateJsonResponse(tx_obj);
      });
}

}  // namespace ledger
}  // namespace fetch
