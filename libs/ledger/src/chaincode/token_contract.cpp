#include "ledger/chaincode/token_contract.hpp"
#include "core/script/variant.hpp"
#include "core/byte_array/decoders.hpp"
#include "crypto/fnv.hpp"

#include <stdexcept>

namespace fetch {
namespace ledger {
namespace  {


struct WalletRecord {
  uint64_t balance{0};

  template <typename T>
  friend void Serialize(T &serializer, WalletRecord const &b) {
    serializer << b.balance;
  }

  template <typename T>
  friend void Deserialize(T &serializer, WalletRecord &b) {
    serializer >> b.balance;
  }
};

} // namespace

TokenContract::TokenContract()
  : Contract("fetch.token") {
  OnTransaction("wealth", this, &TokenContract::CreateWealth);
  OnTransaction("transfer", this, &TokenContract::Transfer);
  OnQuery("balance", this, &TokenContract::Balance);
}

Contract::Status TokenContract::CreateWealth(transaction_type const &tx) {

  script::Variant data;
  if (!ParseAsJson(tx, data)) {
    return Status::FAILED;
  }

  byte_array::ByteArray address;
  uint64_t amount = 0;
  if (Extract(data, "address", address) && Extract(data, "amount", amount)) {
    address = byte_array::FromBase64(address); //  the address needs to be converted

    // retrieve the record (if it exists)
    WalletRecord record{};
    GetOrCreateStateRecord(record, address);

    // update the balance
    record.balance += amount;

    SetStateRecord(record, address);
  } else {
    return Status::FAILED;
  }

  return Status::OK;
}

Contract::Status TokenContract::Transfer(transaction_type const &tx) {

  script::Variant data;
  if (!ParseAsJson(tx, data)) {
    return Status::FAILED;
  }

  byte_array::ByteArray to_address;
  byte_array::ByteArray from_address;
  uint64_t amount = 0;
  if (Extract(data, "from", from_address) &&
      Extract(data, "to", to_address) &&
      Extract(data, "amount", amount)) {

    to_address = byte_array::FromBase64(to_address); //  the address needs to be converted
    from_address = byte_array::FromBase64(from_address); //  the address needs to be converted

    WalletRecord to_record{};
    WalletRecord from_record{};

    if (!GetStateRecord(from_record, from_address))
      return Status::FAILED;

    // check the balance here to limit further reads if required
    if (from_record.balance < amount)
      return Status::FAILED;

    if (!GetOrCreateStateRecord(to_record, to_address))
      return Status::FAILED;

    // update the records
    from_record.balance -= amount;
    to_record.balance += amount;

    // write the records back to the state database
    SetStateRecord(from_record, from_address);
    SetStateRecord(to_record, to_address);
  }

  return Status::OK;
}

Contract::Status TokenContract::Balance(query_type const &query, query_type &response) {
  Status status = Status::FAILED;

  byte_array::ByteArray address;
  if (Extract(query, "address", address)) {
    address = byte_array::FromBase64(address);

    // lookup the record
    WalletRecord record{};
    GetStateRecord(record, address);

    // formulate the response
    response.MakeObject();
    response["balance"] = record.balance;

    status = Status::OK;
  } else {
    logger.Warn("Incorrect parameters to balance query");
  }

  return status;
}

} // namespace ledger
} // namespace fetch
