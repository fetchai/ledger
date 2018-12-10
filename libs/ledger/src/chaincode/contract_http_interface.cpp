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

#include "ledger/chaincode/contract_http_interface.hpp"

namespace fetch {
namespace ledger {
namespace {

struct AdaptedTx
{
  chain::MutableTransaction                          tx;
  chain::TxSigningAdapter<chain::MutableTransaction> adapter{tx};

  template <typename T>
  friend void Deserialize(T &serializer, AdaptedTx &tx)
  {
    serializer >> tx.adapter;
  }
};

byte_array::ConstByteArray const API_PATH_CONTRACT_PREFIX("/api/contract/");
byte_array::ConstByteArray const CONTRACT_NAME_SEPARATOR(".");
byte_array::ConstByteArray const PATH_SEPARATOR("/");

http::HTTPResponse JsonBadRequest()
{
  return http::CreateJsonResponse("", http::Status::CLIENT_ERROR_BAD_REQUEST);
}

}  // namespace

constexpr char const *ContractHttpInterface::LOGGING_NAME;

ContractHttpInterface::ContractHttpInterface(StorageInterface &    storage,
                                             TransactionProcessor &processor)
  : storage_{storage}
  , processor_{processor}
{
  byte_array::ByteArray contract_full_name;

  // create all the contracts
  auto const &contracts = contract_cache_.factory().GetContracts();
  for (auto const &contract_name : contracts)
  {
    contract_full_name.Resize(0, ResizeParadigm::ABSOLUTE);
    contract_full_name.Append(contract_name, CONTRACT_NAME_SEPARATOR);
    std::size_t const contract_full_name_base_size = contract_full_name.size();

    // create the contract
    auto contract = contract_cache_.factory().Create(contract_name);

    byte_array::ByteArray contract_path{contract_name};
    contract_path.Replace(static_cast<char const &>(CONTRACT_NAME_SEPARATOR[0]),
                          static_cast<char const &>(PATH_SEPARATOR[0]));

    byte_array::ByteArray api_path;
    //* ByteArry from `contract_name` performs deep copy due to const -> non-const
    api_path.Append(API_PATH_CONTRACT_PREFIX, contract_path, PATH_SEPARATOR);
    std::size_t const api_path_base_size = api_path.size();

    // enumerate all of the contract query handlers
    auto const &query_handlers = contract->query_handlers();
    for (auto const &handler : query_handlers)
    {
      byte_array::ConstByteArray const &query_name = handler.first;
      api_path.Resize(api_path_base_size, ResizeParadigm::ABSOLUTE);
      api_path.Append(query_name);

      // FETCH_LOG_INFO(LOGGING_NAME, "API: ", api_path);
      FETCH_LOG_INFO(LOGGING_NAME, "QUERY API HANDLER: ", api_path);

      Post(api_path, [this, contract_name, query_name](http::ViewParameters const &,
                                                       http::HTTPRequest const &request) {
        return OnQuery(contract_name, query_name, request);
      });
    }

    auto const &transaction_handlers = contract->transaction_handlers();
    for (auto const &handler : transaction_handlers)
    {
      byte_array::ConstByteArray const &query_name = handler.first;
      api_path.Resize(api_path_base_size, ResizeParadigm::ABSOLUTE);
      api_path.Append(query_name);

      FETCH_LOG_INFO(LOGGING_NAME, "TX API HANDLER: ", api_path);

      contract_full_name.Resize(contract_full_name_base_size, ResizeParadigm::ABSOLUTE);
      contract_full_name.Append(query_name);

      Post(api_path, [this, contract_full_name](http::ViewParameters const &params,
                                                http::HTTPRequest const &   request) {
        return OnTransaction(params, request, &contract_full_name);
      });
    }
  }

  // add custom debug handlers
  Post("/api/debug/submit", [this](http::ViewParameters const &, http::HTTPRequest const &request) {
    chain::MutableTransaction tx;

    tx.PushResource("foo.bar.baz" + std::to_string(transaction_index_));
    tx.set_fee(transaction_index_);
    tx.set_contract_name("fetch.dummy.run");
    tx.set_data(std::to_string(transaction_index_++));

    processor_.AddTransaction(tx);

    std::ostringstream oss;
    oss << R"({ "submitted": true })";

    return http::CreateJsonResponse(oss.str());
  });

  // TODO(issue 414): Remove legacy/unused/legacy endpoints from public HTTP interface
  Post("/api/contract/fetch/token/submit",
       [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
         return OnTransaction(params, request);
       });

  // TODO(issue 414): Remove legacy/unused/legacy endpoints from public HTTP interface
  Post("/api/contract/submit",
       [this](http::ViewParameters const &params, http::HTTPRequest const &request) {
         return OnTransaction(params, request);
       });
}

http::HTTPResponse ContractHttpInterface::OnQuery(byte_array::ConstByteArray const &contract_name,
                                                  byte_array::ConstByteArray const &query,
                                                  http::HTTPRequest const &         request)
{
  try
  {
    FETCH_LOG_INFO(LOGGING_NAME, "OnQuery");
    FETCH_LOG_DEBUG(LOGGING_NAME, request.body());

    // parse the incoming request
    json::JSONDocument doc;
    doc.Parse(request.body());

    // dispatch the contract type
    variant::Variant response;
    auto             contract = contract_cache_.Lookup(contract_name);

    // attach, dispatch and detach
    contract->Attach(storage_);
    auto const status = contract->DispatchQuery(query, doc.root(), response);
    contract->Detach();

    if (status == Contract::Status::OK)
    {
      // encode the response
      std::ostringstream oss;
      oss << response;

      // generate the response object
      return http::CreateJsonResponse(oss.str());
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Error running query. status = ", static_cast<int>(status));
    }
  }
  catch (std::exception &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Query error: ", ex.what());
  }

  return JsonBadRequest();
}

http::HTTPResponse ContractHttpInterface::OnTransaction(
    http::ViewParameters const &, http::HTTPRequest const &request,
    byte_array::ConstByteArray const *const expected_contract_name)
{
  std::ostringstream oss;
  bool               error_response{true};
  try
  {
    SubmitTxStatus submitted;

    // detect the content format, defaulting to json
    byte_array::ConstByteArray content_type = "application/json";
    if (request.header().Has("content-type"))
    {
      content_type = request.header()["content-type"];
    }

    // handle the types of transaction
    bool unknown_format = false;
    if (content_type == "application/vnd+fetch.transaction+native")
    {
      submitted = SubmitNativeTx(request, expected_contract_name);
    }
    else if (content_type == "application/vnd+fetch.transaction+json" ||
             content_type == "application/json")
    {
      submitted = SubmitJsonTx(request, expected_contract_name);
    }
    else
    {
      unknown_format = true;
    }

    if (unknown_format)
    {
      // format the message
      std::ostringstream error_msg;
      error_msg << "Unknown content type: " << content_type;

      oss << R"({ "submitted": false, "error": )" << std::quoted(error_msg.str()) << " }";
    }
    else if (submitted.processed != submitted.received)
    {
      // format the message
      std::ostringstream error_msg;
      error_msg << "Some transactions has not submitted due to wrong contract type."
                << content_type;

      oss << R"({ "submitted": false, "count": )" << submitted.processed
          << R"(, "expected_count": )" << submitted.received
          << R"(, "error": "Some transactions have NOT been submitted due to miss-matching contract name."})";
    }
    else
    {
      // success report the statistics
      oss << R"({ "submitted": true, "count": )" << submitted.processed << " }";
      error_response = false;
    }
  }
  catch (std::exception const &ex)
  {
    oss.clear();
    oss << R"({ "submitted": false, "error": ")" << std::quoted(ex.what()) << " }";
  }

  return http::CreateJsonResponse(oss.str(), (error_response)
                                                 ? http::Status::CLIENT_ERROR_BAD_REQUEST
                                                 : http::Status::SUCCESS_OK);
}

ContractHttpInterface::SubmitTxStatus ContractHttpInterface::SubmitJsonTx(
    http::HTTPRequest const &               request,
    byte_array::ConstByteArray const *const expected_contract_name)
{
  std::size_t submitted{0};
  std::size_t expected_count{0};

  // parse the JSON request
  json::JSONDocument doc{request.body()};

  FETCH_LOG_INFO(LOGGING_NAME, "NEW TRANSACTION RECEIVED");
  FETCH_LOG_DEBUG(LOGGING_NAME, request.body());

  if (doc.root().IsArray())
  {
    expected_count = doc.root().size();
    for (std::size_t i = 0, end = doc.root().size(); i < end; ++i)
    {
      auto const &tx_obj = doc[i];

      chain::MutableTransaction tx{chain::FromWireTransaction(tx_obj)};

      if (expected_contract_name && tx.contract_name() != *expected_contract_name)
      {
        continue;
      }

      // add the transaction to the processor
      processor_.AddTransaction(std::move(tx));
      ++submitted;
    }
  }
  else
  {
    expected_count = 1;
    chain::MutableTransaction tx{chain::FromWireTransaction(doc.root())};

    if (!expected_contract_name || tx.contract_name() == *expected_contract_name)
    {
      // add the transaction to the processor
      processor_.AddTransaction(std::move(tx));
      ++submitted;
    }
  }

  return SubmitTxStatus{submitted, expected_count};
}

ContractHttpInterface::SubmitTxStatus ContractHttpInterface::SubmitNativeTx(
    http::HTTPRequest const &               request,
    byte_array::ConstByteArray const *const expected_contract_name)
{
  std::vector<AdaptedTx> transactions;

  serializers::ByteArrayBuffer buffer(request.body());
  buffer >> transactions;

  std::size_t submitted{0};
  for (auto const &input_tx : transactions)
  {
    if (expected_contract_name && input_tx.tx.contract_name() != *expected_contract_name)
    {
      continue;
    }

    processor_.AddTransaction(input_tx.tx);
    ++submitted;
  }

  return SubmitTxStatus{submitted, transactions.size()};
}

}  // namespace ledger
}  // namespace fetch
