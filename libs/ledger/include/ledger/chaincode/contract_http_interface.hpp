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

#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "core/string/replace.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "ledger/chaincode/cache.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/transaction_processor.hpp"
#include "miner/miner_interface.hpp"

#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chain/wire_transaction.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace fetch {
namespace ledger {

class ContractHttpInterface : public http::HTTPModule
{
public:
  static constexpr char const *           LOGGING_NAME = "ContractHttpInterface";
  static byte_array::ConstByteArray const API_PATH_CONTRACT_PREFIX;
  static byte_array::ConstByteArray const CONTRACT_NAME_SEPARATOR;
  static byte_array::ConstByteArray const PATH_SEPARATOR;

  ContractHttpInterface(StorageInterface &storage, TransactionProcessor &processor)
    : storage_{storage}
    , processor_{processor}
  {

    // create all the contracts
    auto const &contracts = contract_cache_.factory().GetContracts();
    for (auto const &contract_name : contracts)
    {

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

        FETCH_LOG_INFO(LOGGING_NAME, "API: ", api_path);

        Post(api_path, [this, contract_name, query_name](http::ViewParameters const &,
                                                         http::HTTPRequest const &request) {
          return OnQuery(contract_name, query_name, request);
        });
      }
    }

    // add custom debug handlers
    Post("/api/debug/submit",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           chain::MutableTransaction tx;

           tx.PushResource("foo.bar.baz" + std::to_string(transaction_index_));
           tx.set_fee(transaction_index_);
           tx.set_contract_name("fetch.dummy.run");
           tx.set_data(std::to_string(transaction_index_++));

           auto vtx = chain::VerifiedTransaction::Create(std::move(tx));

           processor_.AddTransaction(vtx);

           std::ostringstream oss;
           oss << R"({ "submitted": true, "tx": ")"
               << static_cast<std::string>(byte_array::ToBase64(vtx.digest())) << "\" }";

           return http::CreateJsonResponse(oss.str());
         });

    // new transaction
    Post("/api/contract/submit",
         [this](http::ViewParameters const &, http::HTTPRequest const &request) {
           std::ostringstream oss;
           std::size_t submitted{0};

           try
           {
             // parse the JSON request
             json::JSONDocument doc{request.body()};

             if (doc.root().is_array())
             {
               for (std::size_t i = 0, end = doc.root().size(); i < end; ++i)
               {
                 auto const &tx_obj = doc[i];

                 // assume single transaction
                 auto tx = chain::VerifiedTransaction::Create(chain::FromWireTransaction(tx_obj));

                 // add the transaction to the processor
                 processor_.AddTransaction(tx);
                 ++submitted;
               }
             }
             else
             {
               // assume single transaction
               auto tx = chain::VerifiedTransaction::Create(chain::FromWireTransaction(doc.root()));

               // add the transaction to the processor
               processor_.AddTransaction(tx);
               ++submitted;
             }

             oss << R"({ "submitted": true, "count": )" << submitted << " }";
           }
           catch (std::exception const &ex)
           {
             oss.clear();
             oss << R"({ "submitted": false, "error": ")" << ex.what() << "\" }";
           }

           return http::CreateJsonResponse(oss.str());
         });
  }

private:
  http::HTTPResponse OnQuery(byte_array::ConstByteArray const &contract_name,
                             byte_array::ConstByteArray const &query,
                             http::HTTPRequest const &         request)
  {
    try
    {
      // parse the incoming request
      json::JSONDocument doc;
      doc.Parse(request.body());

      // dispatch the contract type
      script::Variant response;
      auto            contract = contract_cache_.Lookup(contract_name);

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

  static http::HTTPResponse JsonBadRequest()
  {
    return http::CreateJsonResponse("", http::Status::CLIENT_ERROR_BAD_REQUEST);
  }

  std::size_t transaction_index_{0};

  StorageInterface &    storage_;
  TransactionProcessor &processor_;
  ChainCodeCache        contract_cache_;
};

}  // namespace ledger
}  // namespace fetch
