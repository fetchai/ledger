#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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

#include <sstream>

#include <iostream>

namespace fetch {
namespace ledger {

class ContractHttpInterface : public http::HTTPModule
{
public:
  ContractHttpInterface(StorageInterface &storage, TransactionProcessor &processor)
    : storage_{storage}, processor_{processor}
  {

    // create all the contracts
    auto const &contracts = contract_cache_.factory().GetContracts();
    for (auto const &contract_name : contracts)
    {

      // create the contract
      auto contract = contract_cache_.factory().Create(contract_name);

      // define the api prefix
      std::string const api_prefix =
          "/api/contract/" + string::Replace(contract_name, '.', '/') + '/';

      // enumerate all of the contract query handlers
      auto const &query_handlers = contract->query_handlers();
      for (auto const &handler : query_handlers)
      {
        std::string const &query_name = handler.first;
        std::string const  api_path   = api_prefix + query_name;

        fetch::logger.Info("API: ", api_path);

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
  }

private:
  http::HTTPResponse OnQuery(std::string const &contract_name, std::string const &query,
                             http::HTTPRequest const &request)
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
        fetch::logger.Warn("Error running query. status = ", static_cast<int>(status));
      }
    }
    catch (std::exception &ex)
    {
      fetch::logger.Warn("Query error: ", ex.what());
    }

    return JsonBadRequest();
  }

  static http::HTTPResponse JsonBadRequest()
  {
    return http::CreateJsonResponse("", http::status_code::CLIENT_ERROR_BAD_REQUEST);
  }

  std::size_t transaction_index_{0};

  StorageInterface &    storage_;
  TransactionProcessor &processor_;
  ChainCodeCache        contract_cache_;
};

}  // namespace ledger
}  // namespace fetch
