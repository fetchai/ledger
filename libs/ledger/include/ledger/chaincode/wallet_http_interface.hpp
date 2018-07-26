#pragma once

#include "core/assert.hpp"
#include "core/byte_array/decoders.hpp"
#include "core/byte_array/encoders.hpp"
#include "core/json/document.hpp"
#include "core/logger.hpp"
#include "http/json_response.hpp"
#include "http/module.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "ledger/transaction_processor.hpp"

#include <random>
#include <sstream>

namespace fetch {
namespace ledger {

class WalletHttpInterface : public http::HTTPModule
{
public:
  enum class ErrorCode
  {
    NOT_IMPLEMENTED = 1000,
    PARSE_FAILURE
  };

  WalletHttpInterface(StateInterface &state, TransactionProcessor &processor)
      : state_{state}, processor_{processor}
  {

    // register all the routes
    Post("/register", [this](http::ViewParameters const &,
                             http::HTTPRequest const &request) {
      return OnRegister(request);
    });

    Post("/balance", [this](http::ViewParameters const &,
                            http::HTTPRequest const &request) {
      return OnBalance(request);
    });

    Post("/transfer", [this](http::ViewParameters const &,
                             http::HTTPRequest const &request) {
      return OnTransfer(request);
    });

    Post("/transactions", [this](http::ViewParameters const &,
                                 http::HTTPRequest const &request) {
      return OnTransactions(request);
    });
  }

private:
  http::HTTPResponse OnRegister(http::HTTPRequest const &request)
  {
    static constexpr std::size_t IDENTITY_SIZE = 64;

    std::random_device rd;
    std::mt19937       rng(rd());

    byte_array::ByteArray address;
    address.Resize(IDENTITY_SIZE);
    for (std::size_t i = 0; i < IDENTITY_SIZE; ++i)
    {
      address[i] = static_cast<uint8_t>(rng() & 0xFF);
    }

    script::Variant data;
    data.MakeObject();
    data["address"] = byte_array::ToBase64(address);
    data["success"] = true;

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
      mtx.PushResource(address);

      // dispatch the transaction
      processor_.AddTransaction(
          chain::VerifiedTransaction::Create(std::move(mtx)));
    }

    std::ostringstream oss;
    oss << data;
    return http::CreateJsonResponse(oss.str(), http::status_code::SUCCESS_OK);
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

      return http::CreateJsonResponse(oss.str(), http::status_code::SUCCESS_OK);
    }
    catch (json::JSONParseException const &ex)
    {
      fetch::logger.Warn("Failed to parse input balance request: ", ex.what());
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
      uint64_t              amount;

      // extract all the requeset parameters
      if (script::Extract(doc.root(), "from", from) &&
          script::Extract(doc.root(), "to", to) &&
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

        // create the final / sealed transaction
        chain::VerifiedTransaction tx =
            chain::VerifiedTransaction::Create(std::move(mtx));

        // dispatch to the wider system
        processor_.AddTransaction(tx);

        return http::CreateJsonResponse(R"({"success": true})",
                                        http::status_code::SUCCESS_OK);
      }
    }
    catch (json::JSONParseException const &ex)
    {
      fetch::logger.Warn("Failed to parse input transfer request: ", ex.what());
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
        << R"("error_code": )" << static_cast<int>(error_code) << ','
        << R"("message": )" << ToString(error_code) << '}';

    return http::CreateJsonResponse(
        oss.str(), http::status_code::CLIENT_ERROR_BAD_REQUEST);
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
  StateInterface &      state_;
  TransactionProcessor &processor_;
};

}  // namespace ledger
}  // namespace fetch
