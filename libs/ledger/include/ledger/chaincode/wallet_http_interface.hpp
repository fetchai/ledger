#ifndef FETCH_WALLET_HTTP_INTERFACE_HPP
#define FETCH_WALLET_HTTP_INTERFACE_HPP

#include "core/json/document.hpp"
#include "core/assert.hpp"
#include "core/logger.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "http/module.hpp"
#include "http/json_response.hpp"
#include "ledger/chain/mutable_transaction.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/transaction_processor.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <sstream>

namespace fetch {
namespace ledger {

class WalletHttpInterface : public http::HTTPModule {
public:

  enum class ErrorCode {
    NOT_IMPLEMENTED = 0,
  };

  WalletHttpInterface(StateInterface &state, TransactionProcessor &processor)
    : state_{state}
    , processor_{processor} {

    // register all the routes
    Post("/register", [this](http::ViewParameters const &, http::HTTPRequest const &request) {
      return OnRegister(request);
    });

    Post("/balance", [this](http::ViewParameters const &, http::HTTPRequest const &request) {
      return OnBalance(request);
    });

    Post("/transfer", [this](http::ViewParameters const &, http::HTTPRequest const &request) {
      return OnTransfer(request);
    });

    Post("/transactions", [this](http::ViewParameters const &, http::HTTPRequest const &request) {
      return OnTransactions(request);
    });

    (void) state_;
    (void) processor_;
  }

private:

  http::HTTPResponse OnRegister(http::HTTPRequest const &request) {
    return BadJsonResponse(ErrorCode::NOT_IMPLEMENTED);
  }

  http::HTTPResponse OnBalance(http::HTTPRequest const &request) {

    try {

      // parse the json request
      json::JSONDocument doc;
      doc.Parse(request.body());

      script::Variant response;
      contract_.DispatchQuery("balance", doc.root(), response);

      std::ostringstream oss;
      oss << response;

      return http::CreateJsonResponse(oss.str(), http::status_code::CLIENT_ERROR_BAD_REQUEST);
    } catch (json::JSONParseException const &ex) {
      fetch::logger.Warn("Failed to parse input balance request: ", ex.what());
    }

    return BadJsonResponse(ErrorCode::NOT_IMPLEMENTED);
  }

  http::HTTPResponse OnTransfer(http::HTTPRequest const &request) {

    // parse the json request
    json::JSONDocument doc;
    doc.Parse(request.body());

    byte_array::ByteArray from;
    byte_array::ByteArray to;
    uint64_t amount;

    // extract all the requeset parameters
    if (script::Extract(doc.root(), "from", from) &&
        script::Extract(doc.root(), "to", to) &&
        script::Extract(doc.root(), "amount", amount) ) {

      script::Variant data;
      data.MakeObject();
      data["from"] = "foo";
      data["to"] = "bar";
      data["amount"] = 10;

      std::ostringstream oss;
      oss << data;

      chain::MutableTransaction mtx;
      mtx.set_contract_name("fetch.token.transfer");
      mtx.set_data(oss.str());
      mtx.PushResource("foo");
      mtx.PushResource("bar");

      // create the final / sealed transaction
      chain::VerifiedTransaction tx = chain::VerifiedTransaction::Create(std::move(mtx));

      processor_.AddTransaction(tx);
    }



    return BadJsonResponse(ErrorCode::NOT_IMPLEMENTED);
  }

  http::HTTPResponse OnTransactions(http::HTTPRequest const &request) {
    return BadJsonResponse(ErrorCode::NOT_IMPLEMENTED);
  }

  static http::HTTPResponse BadJsonResponse(ErrorCode error_code) {

    std::ostringstream oss;
    oss << '{'
        << R"("success": false,)"
        << R"("error_code": )" << static_cast<int>(error_code) << ','
        << R"("message": )" << ToString(error_code)
        << '}';

    return http::CreateJsonResponse(oss.str(), http::status_code::CLIENT_ERROR_BAD_REQUEST);
  }

  static const char *ToString(ErrorCode error_code) {
    char const *msg = "unknown error";

    switch (error_code) {
      case ErrorCode::NOT_IMPLEMENTED:
        msg = "Not implemented";
        break;
    }

    return msg;
  }

  TokenContract contract_;
  StateInterface &state_;
  TransactionProcessor &processor_;
};

} // namespace ledger
} // namespace fetch

#endif //FETCH_WALLET_HTTP_INTERFACE_HPP
