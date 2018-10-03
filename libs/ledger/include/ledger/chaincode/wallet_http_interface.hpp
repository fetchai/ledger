#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include "core/assert.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "crypto/ecdsa.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/transaction_processor.hpp"
#include "storage/object_store.hpp"

#include <random>
#include <sstream>

namespace fetch {
namespace ledger {

class WalletHttpInterface : public http::HTTPModule
{
public:
  using KeyStore               = storage::ObjectStore<byte_array::ConstByteArray>;

  static constexpr char const *LOGGING_NAME = "WalletHttpInterface";

  enum class ErrorCode
  {
    NOT_IMPLEMENTED = 1000,
    PARSE_FAILURE
  };

  WalletHttpInterface(StorageInterface &state, TransactionProcessor &processor, KeyStore &key_store)
    : state_{state}
    , processor_{processor}
    , key_store_{key_store}
  {
    // register all the routes
    Post("/api/wallet/register",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           return OnRegister(request);
         });

    Post("/api/wallet/balance",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           return OnBalance(request);
         });

    Post("/api/wallet/transfer",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           return OnTransfer(request);
         });

    Post("/api/wallet/transactions",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           return OnTransactions(request);
         });
  }

private:
  /**
   * Create address(es) with some amount of wealth and submit it to the network
   *
   * @param: request The http request, optionally used to create more than one address
   *
   * @return: The address(es) of the new wealth wrapped as a HTTP response
   */
  http::HTTPResponse OnRegister(http::HTTPRequest const &request)
  {
    // Determine number of locations to create
    uint64_t count = 1;

    {
      json::JSONDocument doc;
      doc = request.JSON();

      count = doc["count"].As<uint64_t>();
    }

    // Avoid locations = 0 corner case
    count = count == 0 ? 1 : count;

    // Cap locations
    count = std::min(count, uint64_t(10000));

    std::vector<crypto::ECDSASigner> signers(count);
        
    std::random_device rd;
    std::mt19937       rng(rd());

    for (auto &signer : signers)
    {
      signer.GenerateKeys();

      // Create random address
      byte_array::ConstByteArray const &address{ signer.public_key() };

      // construct the wealth generation transaction
      {
        script::Variant wealth_data;
        wealth_data.MakeObject();
        wealth_data["address"] = byte_array::ToBase64(address);
        wealth_data["amount"]  = 1000;

        std::ostringstream oss;
        oss << wealth_data;

        chain::MutableTransaction mtx;
        mtx.set_contract_name("fetch.token.wealth");
        mtx.set_data(oss.str());
        mtx.set_fee(rng() & 0x1FF);
        mtx.PushResource(address);

        // sign the transaction
        auto tx_sign_adapter{chain::TxSigningAdapterFactory(mtx)};
        mtx.Sign(signer.private_key(), tx_sign_adapter);

        FETCH_LOG_DEBUG(LOGGING_NAME, "Submitting register transaction");

        // dispatch the transaction
        processor_.AddTransaction(chain::VerifiedTransaction::Create(std::move(mtx)));
      }
      
      key_store_.Set(storage::ResourceAddress{address}, signer.private_key());
    }

    script::Variant    data;
    std::ostringstream oss;

    // Return old data format as a fall back (when size is 1)
    if (signers.size() == 1)
    {
      data.MakeObject();

      data["address"] = byte_array::ToBase64(signers[0].public_key());
      data["success"] = true;
    }
    else
    {
      data.MakeObject();
      data["success"] = true;

      script::Variant results_array;
      results_array.MakeArray(signers.size());

      std::size_t index = 0;
      for (auto const &signer : signers)
      {
        script::Variant element;
        element.MakeObject();
        element["address"]   = byte_array::ToBase64(signer.public_key());
        results_array[index++] = element;
      }

      data["addresses"] = results_array;
    }

    oss << data;
    return http::CreateJsonResponse(oss.str(), http::Status::SUCCESS_OK);
  }

  http::HTTPResponse OnBalance(http::HTTPRequest const &request)
  {
    try
    {
      // parse the json request
      json::JSONDocument doc;
      doc.Parse(request.body());

      script::Variant response;
      contract_.Attach(state_);
      contract_.DispatchQuery("balance", doc.root(), response);
      contract_.Detach();

      std::ostringstream oss;
      oss << response;

      return http::CreateJsonResponse(oss.str(), http::Status::SUCCESS_OK);
    }
    catch (json::JSONParseException const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input balance request: ", ex.what());
    }

    return BadJsonResponse(ErrorCode::PARSE_FAILURE);
  }

  http::HTTPResponse OnTransfer(http::HTTPRequest const &request)
  {
    try
    {
      // parse the json request
      json::JSONDocument doc;
      doc.Parse(request.body());

      byte_array::ByteArray from;
      byte_array::ByteArray to;
      uint64_t              amount = 0;

      // extract all the request parameters
      if (script::Extract(doc.root(), "from", from) && script::Extract(doc.root(), "to", to) &&
          script::Extract(doc.root(), "amount", amount))
      {

        script::Variant data;
        data.MakeObject();
        data["from"]   = from;
        data["to"]     = to;
        data["amount"] = amount;

        // convert the data into json
        std::ostringstream oss;
        oss << data;

        // build up the transfer transaction
        chain::MutableTransaction mtx;
        mtx.set_contract_name("fetch.token.transfer");
        mtx.set_data(oss.str());
        mtx.PushResource(byte_array::FromBase64(from));
        mtx.PushResource(byte_array::FromBase64(to));

        // query private key for signing
        byte_array::ConstByteArray priv_key;
        if ( !key_store_.Get(storage::ResourceAddress{from}, priv_key))
        {
          return http::CreateJsonResponse(R"({"success": false, "error": "provided address/pub.key does not exist in key store"})", http::Status::CLIENT_ERROR_BAD_REQUEST);
        }

        // sign the transaction
        auto tx_sign_adapter{chain::TxSigningAdapterFactory(mtx)};
        mtx.Sign(priv_key, tx_sign_adapter);

        // create the final / sealed transaction
        chain::VerifiedTransaction tx = chain::VerifiedTransaction::Create(std::move(mtx));

        // dispatch to the wider system
        processor_.AddTransaction(tx);

        return http::CreateJsonResponse(R"({"success": true})", http::Status::SUCCESS_OK);
      }
    }
    catch (json::JSONParseException const &ex)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input transfer request: ", ex.what());
    }

    return BadJsonResponse(ErrorCode::PARSE_FAILURE);
  }

  http::HTTPResponse OnTransactions(http::HTTPRequest const &request)
  {
    return BadJsonResponse(ErrorCode::NOT_IMPLEMENTED);
  }

  static http::HTTPResponse BadJsonResponse(ErrorCode error_code)
  {

    std::ostringstream oss;
    oss << '{' << R"("success": false,)"
        << R"("error_code": )" << static_cast<int>(error_code) << ',' << R"("message": )"
        << ToString(error_code) << '}';

    return http::CreateJsonResponse(oss.str(), http::Status::CLIENT_ERROR_BAD_REQUEST);
  }

  static const char *ToString(ErrorCode error_code)
  {
    char const *msg = "unknown error";

    switch (error_code)
    {
    case ErrorCode::NOT_IMPLEMENTED:
      msg = "Not implemented";
      break;
    case ErrorCode::PARSE_FAILURE:
      msg = "Parse failure";
      break;
    }

    return msg;
  }

  TokenContract         contract_;
  StorageInterface &    state_;
  TransactionProcessor &processor_;
  KeyStore &            key_store_;
};

}  // namespace ledger
}  // namespace fetch
