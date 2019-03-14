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

#include "core/assert.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "crypto/ecdsa.hpp"
#include "http/json_response.hpp"
#include "ledger/chain/helper_functions.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/state_adapter.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/transaction_processor.hpp"
#include "storage/object_store.hpp"
#include "variant/variant_utils.hpp"

#include "miner/resource_mapper.hpp"

#include "ledger/chaincode/wallet_http_interface.hpp"

#include <random>
#include <sstream>
#include <utility>

namespace fetch {
namespace ledger {

WalletHttpInterface::WalletHttpInterface(StorageInterface &state, TransactionProcessor &processor,
                                         std::size_t num_lanes)
  : state_{state}
  , processor_{processor}
  , num_lanes_{num_lanes}
{

  log2_lanes_ =
      uint32_t((sizeof(uint32_t) << 3) - uint32_t(__builtin_clz(uint32_t(num_lanes_)) + 1));

  // load permanent key store (or create it if files do not exist)
  key_store_.Load("key_store_main.db", "key_store_index.db", true);

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

/**
 * Create address(es) with some amount of wealth and submit it to the network
 *
 * @param: request The http request, optionally used to create more than one address
 *
 * @return: The address(es) of the new wealth wrapped as a HTTP response
 */
http::HTTPResponse WalletHttpInterface::OnRegister(http::HTTPRequest const &request)
{
  // Determine number of locations to create
  uint64_t count = 1;

  try
  {
    json::JSONDocument doc = request.JSON();

    auto const &count_v = doc["count"];
    if (count_v.Is<uint64_t>())
    {
      count = count_v.As<uint64_t>();
    }
  }
  catch (json::JSONParseException &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Json Parse failure: ", ex.what());
    return BadJsonResponse(ErrorCode::PARSE_FAILURE);
  }

  // Avoid locations = 0 corner case
  count = count == 0 ? 1 : count;

  // Cap locations
  count = std::min(count, uint64_t(10000));

  std::vector<crypto::ECDSASigner>          signers(count);
  std::vector<std::tuple<std::string, int>> return_info;

  std::random_device rd;
  std::mt19937       rng(rd());

  for (auto &signer : signers)
  {
    signer.GenerateKeys();

    // Create random address
    byte_array::ConstByteArray const &address{signer.public_key()};

    // construct the wealth generation transaction
    {
      variant::Variant wealth_data = variant::Variant::Object();
      wealth_data["address"]       = byte_array::ToBase64(address);
      wealth_data["amount"]        = 1001;

      std::ostringstream oss;
      oss << wealth_data;

      ledger::MutableTransaction mtx;
      mtx.set_contract_name("fetch.token.wealth");
      mtx.set_data(oss.str());
      mtx.set_fee(rng() & 0x1FF);
      mtx.PushResource(address);

      // sign the transaction
      mtx.Sign(signer.private_key());

      uint32_t lane = miner::MapResourceToLane(address, mtx.contract_name(), log2_lanes_);

      std::tuple<std::string, int> tmp =
          std::make_tuple(std::string(byte_array::ToBase64(signer.public_key())), lane);
      return_info.push_back(tmp);

      FETCH_LOG_DEBUG(LOGGING_NAME, "Submitting register transaction");

      // dispatch the transaction
      processor_.AddTransaction(std::move(mtx));
    }

    storage::ResourceID set_id = storage::ResourceAddress{address};
    key_store_.Set(set_id, signer.private_key());
  }

  variant::Variant   data;
  std::ostringstream oss;

  // Return old data format as a fall back (when size is 1)
  if (return_info.size() == 1)
  {
    data = variant::Variant::Object();

    data["address"] = std::get<0>(return_info[0]);
    data["success"] = true;
  }
  else
  {
    data            = variant::Variant::Object();
    data["success"] = true;

    variant::Variant &results_array = data["addresses"];
    results_array                   = variant::Variant::Array(return_info.size());

    std::size_t index{0};
    for (auto const &info : return_info)
    {
      variant::Variant &metadata = results_array[index++];
      metadata                   = variant::Variant::Object();

      metadata["address"] = std::get<0>(info);
      metadata["lane"]    = std::get<1>(info);
    }
  }

  oss << data;
  return http::CreateJsonResponse(oss.str(), http::Status::SUCCESS_OK);
}

http::HTTPResponse WalletHttpInterface::OnBalance(http::HTTPRequest const &request)
{
  try
  {
    TokenContract contract;

    // parse the json request
    json::JSONDocument doc;
    doc.Parse(request.body());

    variant::Variant response;

    StateAdapter adapter{state_, Identifier{TokenContract::NAME}};
    contract.Attach(adapter);
    contract.DispatchQuery("balance", doc.root(), response);
    contract.Detach();

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

http::HTTPResponse WalletHttpInterface::OnTransfer(http::HTTPRequest const &request)
{
  try
  {
    // parse the json request
    json::JSONDocument doc;
    doc.Parse(request.body());

    byte_array::ConstByteArray from;
    byte_array::ConstByteArray to;
    uint64_t                   amount = 0;

    // extract all the request parameters
    if (variant::Extract(doc.root(), "from", from) && variant::Extract(doc.root(), "to", to) &&
        variant::Extract(doc.root(), "amount", amount))
    {

      variant::Variant data = variant::Variant::Object();
      data["from"]          = from;
      data["to"]            = to;
      data["amount"]        = amount;

      // convert the data into json
      std::ostringstream oss;
      oss << data;

      // build up the transfer transaction
      ledger::MutableTransaction mtx;
      mtx.set_contract_name("fetch.token.transfer");
      mtx.set_data(oss.str());
      mtx.PushResource(byte_array::FromBase64(from));
      mtx.PushResource(byte_array::FromBase64(to));

      storage::ResourceAddress get_id{byte_array::FromBase64(from)};

      // query private key for signing
      byte_array::ConstByteArray priv_key;
      if (!key_store_.Get(get_id, priv_key))
      {
        return http::CreateJsonResponse(
            R"({"success": false, "error": "provided address/pub.key does not exist in key store"})",
            http::Status::CLIENT_ERROR_BAD_REQUEST);
      }

      // sign the transaction
      auto tx_sign_adapter{ledger::TxSigningAdapterFactory(mtx)};

      mtx.Sign(priv_key, tx_sign_adapter);

      // dispatch to the wider system
      processor_.AddTransaction(std::move(mtx));

      return http::CreateJsonResponse(R"({"success": true})", http::Status::SUCCESS_OK);
    }
  }
  catch (json::JSONParseException const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse input transfer request: ", ex.what());
  }

  return BadJsonResponse(ErrorCode::PARSE_FAILURE);
}

http::HTTPResponse WalletHttpInterface::OnTransactions(http::HTTPRequest const & /*request*/)
{
  return BadJsonResponse(ErrorCode::NOT_IMPLEMENTED);
}

http::HTTPResponse WalletHttpInterface::BadJsonResponse(ErrorCode error_code)
{

  std::ostringstream oss;
  oss << '{' << R"("success": false,)"
      << R"("error_code": )" << static_cast<int>(error_code) << ',' << R"("message": )"
      << ToString(error_code) << '}';

  return http::CreateJsonResponse(oss.str(), http::Status::CLIENT_ERROR_BAD_REQUEST);
}

const char *WalletHttpInterface::ToString(ErrorCode error_code)
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

}  // namespace ledger
}  // namespace fetch
