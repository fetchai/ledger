#include "ledger/chaincode/contract.hpp"
#include "core/json/document.hpp"

namespace fetch {
namespace ledger {

bool Contract::ParseAsJson(transaction_type const &tx, script::Variant &output) {
  bool success = false;

  json::JSONDocument document;

  try {
    // parse the data of the transaction
    document.Parse(tx.data());
    success = true;
  } catch(json::JSONParseException &ex) {
    // expected
  }

  if (success) {
    output = document.root();
  }

  return success;
}

} // namespace ledger
} // namespace fetch
