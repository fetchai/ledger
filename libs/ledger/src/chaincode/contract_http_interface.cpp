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

#include "chain/json_transaction.hpp"
#include "chain/transaction.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/serializers/main_serializer.hpp"
#include "http/json_response.hpp"
#include "json/document.hpp"
#include "ledger/chaincode/chain_code_factory.hpp"
#include "ledger/chaincode/contract.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/contract_context_attacher.hpp"
#include "ledger/chaincode/contract_http_interface.hpp"
#include "ledger/state_adapter.hpp"
#include "ledger/transaction_processor.hpp"
#include "logging/logging.hpp"
#include "variant/variant.hpp"

#include <ctime>
#include <exception>
#include <iomanip>
#include <memory>
#include <string>
#include <utility>

namespace fetch {
namespace ledger {
namespace {

using fetch::byte_array::ByteArray;
using fetch::byte_array::ConstByteArray;
using fetch::chain::FromJsonTransaction;
using fetch::variant::Variant;

ConstByteArray const API_PATH_CONTRACT_PREFIX("/api/contract/");
ConstByteArray const CONTRACT_NAME_SEPARATOR(".");
ConstByteArray const PATH_SEPARATOR("/");

ConstByteArray Quoted(std::string const &value)
{
  std::ostringstream oss;
  oss << std::quoted(value);
  return oss.str();
}

ConstByteArray Quoted(ConstByteArray const &value)
{
  return Quoted(static_cast<std::string>(value));
}

ConstByteArray Quoted(char const *value)
{
  return Quoted(std::string{value});
}

http::HTTPResponse JsonBadRequest()
{
  return http::CreateJsonResponse("", http::Status::CLIENT_ERROR_BAD_REQUEST);
}

std::string GenerateTimestamp()
{
  std::time_t now = std::time(nullptr);

  char buffer[60] = {0};
  std::strftime(buffer, sizeof(buffer), "%FT%TZ", gmtime(&now));

  return {buffer};
}

bool CreateTxFromJson(Variant const &tx_obj, std::vector<ConstByteArray> &txs,
                      TransactionProcessor &processor)
{
  auto tx = std::make_shared<chain::Transaction>();

  if (chain::FromJsonTransaction(tx_obj, *tx))
  {
    if (tx->charge_limit() > chain::Transaction::MAXIMUM_TX_CHARGE_LIMIT)
    {
      return false;
    }

    txs.emplace_back(tx->digest());
    processor.AddTransaction(std::move(tx));

    return true;
  }

  return false;
}

bool CreateTxFromBuffer(ConstByteArray const &encoded_tx, std::vector<ConstByteArray> &txs,
                        TransactionProcessor &processor)
{
  auto tx = std::make_shared<chain::Transaction>();

  chain::TransactionSerializer tx_serializer{encoded_tx};
  if (tx_serializer.Deserialize(*tx))
  {
    if (tx->charge_limit() > chain::Transaction::MAXIMUM_TX_CHARGE_LIMIT)
    {
      return false;
    }

    txs.emplace_back(tx->digest());
    processor.AddTransaction(std::move(tx));

    return true;
  }

  return false;
}

constexpr char const *LOGGING_NAME = "ContractHttpInterface";

}  // namespace

/**
 * Construct an Contract HTTP Interface module
 *
 * @param storage The reference to the storage engine
 * @param processor The reference to the (input) transaction processor
 */
ContractHttpInterface::ContractHttpInterface(StorageInterface &    storage,
                                             TransactionProcessor &processor)
  : storage_{storage}
  , processor_{processor}
  , access_log_{"access.log"}
{
  // create all the contracts
  auto const &contracts = GetChainCodeContracts();
  for (auto const &contract_name : contracts)
  {
    // create the contract
    auto contract = CreateChainCode(contract_name);

    ByteArray contract_path{contract_name};
    contract_path.Replace(static_cast<char const &>(CONTRACT_NAME_SEPARATOR[0]),
                          static_cast<char const &>(PATH_SEPARATOR[0]));

    // enumerate all of the contract query handlers
    auto const &query_handlers = contract->query_handlers();
    for (auto const &handler : query_handlers)
    {
      ConstByteArray const &query_name = handler.first;

      // build up the API path
      ByteArray api_path;
      api_path.Append(API_PATH_CONTRACT_PREFIX, contract_path, PATH_SEPARATOR, query_name);

      FETCH_LOG_INFO(LOGGING_NAME, "Query API: ", api_path);

      Post(api_path, "Calls contract query " + query_name + " for " + contract_name,
           [this, contract_name, query_name](http::ViewParameters const &,
                                             http::HTTPRequest const &request) {
             return OnQuery(contract_name, query_name, request);
           });
    }

    auto const &transaction_handlers = contract->transaction_handlers();
    for (auto const &handler : transaction_handlers)
    {
      ConstByteArray const &transaction_name = handler.first;

      // build up the API path
      ByteArray api_path;
      api_path.Append(API_PATH_CONTRACT_PREFIX, contract_path, PATH_SEPARATOR, transaction_name);

      // build up the canonical contract name
      ByteArray canonical_contract_name;
      canonical_contract_name.Append(contract_name, CONTRACT_NAME_SEPARATOR, transaction_name);

      FETCH_LOG_INFO(LOGGING_NAME, "   Tx API: ", api_path, " : ", canonical_contract_name);

      Post(api_path, "Calls contract action " + transaction_name + " for " + contract_name,
           [this, canonical_contract_name](http::ViewParameters const &params,
                                           http::HTTPRequest const &   request) {
             FETCH_UNUSED(params);
             return OnTransaction(request, canonical_contract_name);
           });
    }
  }

  Post("/api/contract/(identifier=[1-9A-HJ-NP-Za-km-z]{48,50})/(query=.+)",
       "Submits a query to a contract",
       {{"identifier", "The query identifier.", http::validators::StringValue()}},
       [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
         // build the contract name
         auto const contract_name = params["identifier"];

         // proxy the call to the query handler
         return OnQuery(contract_name, params["query"], request);
       });

  Post("/api/contract/submit", "Submits a new contract to the ledger.",
       [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
         FETCH_UNUSED(params);
         return OnTransaction(request, ConstByteArray{});
       });
}

/**
 * Contract Query Handler
 *
 * @param contract_name The contract name
 * @param query  The query name
 * @param request The originating HTTPRequest object
 * @return The appropriate HTTPResponse to be returned to the client
 */
http::HTTPResponse ContractHttpInterface::OnQuery(ConstByteArray const &   contract_name,
                                                  ConstByteArray const &   query,
                                                  http::HTTPRequest const &request)
{
  try
  {
    FETCH_LOG_DEBUG(LOGGING_NAME, "Query: ", contract_name, '.', query,
                    " from: ", request.originating_address(), ':', request.originating_port());
    FETCH_LOG_DEBUG(LOGGING_NAME, request.body());

    // record an entry in the access log
    RecordQuery(contract_name, query, request);

    // parse the incoming request
    json::JSONDocument doc;
    doc.Parse(request.body());
    variant::Variant response;
    // dispatch the contract type
    auto contract = contract_cache_.Lookup(contract_name, storage_);

    if (!contract)
    {
      response           = Variant::Object();
      response["status"] = "failed";
      response["msg"]    = "Unable to look up contract: " + static_cast<std::string>(contract_name);
      response["console"] = "";
      response["result"]  = variant::Variant::Null();

      return http::CreateJsonResponse(response, http::Status::CLIENT_ERROR_NOT_FOUND);
    }

    // adapt the storage engine so that that get and sets are sandboxed for the contract
    StateAdapter storage_adapter{storage_, contract_name};

    // Current block index does not apply to queries - set to 0
    Contract::Status status;
    {
      chain::Address address;
      if (contract_name != "fetch.token" && !chain::Address::Parse(contract_name, address))
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Failed to parse address: ", contract_name);
      }
      ContractContext context{&token_contract_, std::move(address), nullptr, &storage_adapter, 0};
      ContractContextAttacher raii(*contract, context);
      status = contract->DispatchQuery(query, doc.root(), response);
    }

    if (Contract::Status::OK == status)
    {
      return http::CreateJsonResponse(response);
    }

    FETCH_LOG_WARN(LOGGING_NAME, "Error running query. status = ", static_cast<int>(status));
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Query error: ", ex.what());
  }

  return JsonBadRequest();
}

/**
 * Transaction handler
 *
 * @param request The originating HTTPRequest object
 * @param expected_contract_name The expected contract name (optional)
 * @return The appropriate HTTPResponse to be returned to the client
 */
http::HTTPResponse ContractHttpInterface::OnTransaction(http::HTTPRequest const &request,
                                                        ConstByteArray const &   expected_contract)
{
  Variant json = Variant::Object();

  try
  {
    SubmitTxStatus submitted{};

    // detect the content format, defaulting to json
    byte_array::ConstByteArray content_type = "application/json";
    if (request.header().Has("content-type"))
    {
      content_type = request.header()["content-type"];
    }

    // handle the types of transaction
    TxHashes txs{};
    bool     unknown_format = true;
    if (content_type == "application/vnd+fetch.transaction+json" ||
        content_type == "application/vnd.fetch-ai.transaction+json" ||
        content_type == "application/json")
    {
      submitted      = SubmitJsonTx(request, txs);
      unknown_format = false;
    }
    else if (content_type == "application/vnd.fetch-ai.transaction+bulk")
    {
      submitted      = SubmitBulkTx(request, txs);
      unknown_format = false;
    }

    // record the transaction in the access log
    RecordTransaction(submitted, request, expected_contract);

    // update the response with the counts
    json["counts"]              = Variant::Object();
    json["counts"]["submitted"] = submitted.processed;
    json["counts"]["received"]  = submitted.received;
    json["txs"]                 = Variant::Array(txs.size());
    for (std::size_t i = 0; i < txs.size(); ++i)
    {
      json["txs"][i] = ToHex(txs[i]);
    }

    if (unknown_format)
    {
      json["error"] = "Unknown content type: " + Quoted(content_type);
    }
    else if (submitted.processed != submitted.received)
    {
      json["error"] =
          "Some transactions have NOT been submitted due to miss-matching contract name.";
    }
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Exception@OnTransaction: ", ex.what());
    json["error"] = Quoted(ex.what());
  }

  // based on the contents of the response determine the correct status code
  http::Status const status_code =
      json.Has("error") ? http::Status::CLIENT_ERROR_BAD_REQUEST : http::Status::SUCCESS_OK;

  return http::CreateJsonResponse(json, status_code);
}

/**
 * Method handles incoming http request containing single or bulk of JSON formatted
 * Wire transactions.
 *
 * @param request https request containing single or bulk of JSON formatted Wire Transaction(s).
 * @param expected_contract_name contract name each transaction in request must conform to.
 *        Transactions which do NOT conform to this contract name will NOT be accepted further
 *        processing. If the value is `nullptr` (default value) the contract name check is
 *        DISABLED, and so transactions (each received transaction in bulk) can have any contract
 *        name.
 *
 * @return submit status, please see the `SubmitTxStatus` structure
 * @see SubmitTxStatus

 * @see SubmitNativeTx
 */
ContractHttpInterface::SubmitTxStatus ContractHttpInterface::SubmitJsonTx(
    http::HTTPRequest const &request, TxHashes &txs)
{
  std::size_t submitted{0};
  std::size_t expected_count{0};

  // parse the JSON request
  json::JSONDocument doc{request.body()};

  FETCH_LOG_DEBUG(LOGGING_NAME, "NEW TRANSACTION RECEIVED");
  FETCH_LOG_DEBUG(LOGGING_NAME, request.body());

  if (doc.root().IsArray())
  {
    expected_count = doc.root().size();
    for (std::size_t i = 0, end = doc.root().size(); i < end; ++i)
    {
      auto const success = CreateTxFromJson(doc[i], txs, processor_);
      if (success)
      {
        ++submitted;
      }
    }
  }
  else
  {
    expected_count = 1;

    auto const success = CreateTxFromJson(doc.root(), txs, processor_);
    if (success)
    {
      ++submitted;
    }
  }

  FETCH_LOG_DEBUG(LOGGING_NAME, "Submitted ", submitted, " transactions from ",
                  request.originating_address(), ':', request.originating_port());

  return SubmitTxStatus{submitted, expected_count};
}

ContractHttpInterface::SubmitTxStatus ContractHttpInterface::SubmitBulkTx(
    http::HTTPRequest const &request, TxHashes &txs)
{
  std::size_t                 submitted{0};
  std::vector<ConstByteArray> encoded_txs{};

  try
  {
    // extract out all the transaction payloads
    serializers::MsgPackSerializer buffer{request.body()};
    buffer >> encoded_txs;

    for (auto const &encoded_tx : encoded_txs)
    {
      auto const success = CreateTxFromBuffer(encoded_tx, txs, processor_);
      if (success)
      {
        ++submitted;
      }
    }
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_ERROR(LOGGING_NAME, "Error processing bulk tx: ", e.what());
  }

  return SubmitTxStatus{submitted, encoded_txs.size()};
}

/**
 * Record a transaction submission event in the HTTP access log
 *
 * @param status The transaction submission status (counts)
 * @param request The originating HTTPRequest object
 */
void ContractHttpInterface::RecordTransaction(SubmitTxStatus const &   status,
                                              http::HTTPRequest const &request,
                                              ConstByteArray const &   expected_contract)
{
  // form the variant
  Variant entry      = Variant::Object();
  entry["timestamp"] = GenerateTimestamp();
  entry["type"]      = "transaction";
  entry["received"]  = status.received;
  entry["processed"] = status.processed;
  entry["ip"]        = request.originating_address();
  entry["port"]      = request.originating_port();

  if (!expected_contract.empty())
  {
    entry["contract"] = expected_contract;
  }

  // write it out to the access log
  WriteToAccessLog(entry);
}

/**
 * Record a query submission event in the HTTP access log
 *
 * @param contract_name The requested contract name
 * @param query The requested query name
 * @param request The original HTTPRequest object
 */
void ContractHttpInterface::RecordQuery(ConstByteArray const &   contract_name,
                                        ConstByteArray const &   query,
                                        http::HTTPRequest const &request)
{
  Variant entry      = Variant::Object();
  entry["timestamp"] = GenerateTimestamp();
  entry["type"]      = "query";
  entry["contract"]  = contract_name;
  entry["query"]     = query;
  entry["ip"]        = request.originating_address();
  entry["port"]      = request.originating_port();

  // write it out to the access log
  WriteToAccessLog(entry);
}

void ContractHttpInterface::WriteToAccessLog(variant::Variant const &entry)
{
  access_log_.ApplyVoid([&entry](auto &log) { log << entry << '\n'; });
}

}  // namespace ledger
}  // namespace fetch
