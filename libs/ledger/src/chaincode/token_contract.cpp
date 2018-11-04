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

#include "ledger/chaincode/token_contract.hpp"
#include "core/byte_array/decoders.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"
#include "crypto/fnv.hpp"

#include <stdexcept>

static constexpr char const *LOGGING_NAME = "TokenContract";

using fetch::variant::Variant;
using fetch::variant::Extract;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;

namespace fetch {
namespace ledger {
namespace {

/* Implements a record to store wallet contents. */
struct WalletRecord
{
  uint64_t balance{0};

  template <typename T>
  friend void Serialize(T &serializer, WalletRecord const &b)
  {
    serializer << b.balance;
  }

  template <typename T>
  friend void Deserialize(T &serializer, WalletRecord &b)
  {
    serializer >> b.balance;
  }
};

}  // namespace

TokenContract::TokenContract()
  : Contract("fetch.token")
{
  // TODO(tfr): I think the function CreateWealth should be OnInit?
  OnTransaction("wealth", this, &TokenContract::CreateWealth);
  OnTransaction("transfer", this, &TokenContract::Transfer);
  OnQuery("balance", this, &TokenContract::Balance);
}

Contract::Status TokenContract::CreateWealth(Transaction const &tx)
{
  Variant data;
  if (!ParseAsJson(tx, data))
  {
    return Status::FAILED;
  }

  ConstByteArray address;
  uint64_t amount{0};

  if (Extract(data, "address", address) && Extract(data, "amount", amount))
  {
    address = FromBase64(address);  //  the address needs to be converted

    // retrieve the record (if it exists)
    WalletRecord record{};
    GetOrCreateStateRecord(record, address);

    // update the balance
    record.balance += amount;

    SetStateRecord(record, address);
  }
  else
  {
    return Status::FAILED;
  }

  return Status::OK;
}

Contract::Status TokenContract::Transfer(Transaction const &tx)
{
  Variant data;
  if (!ParseAsJson(tx, data))
  {
    return Status::FAILED;
  }

  ConstByteArray to_address;
  ConstByteArray from_address;
  uint64_t       amount{0};
  if (Extract(data, "from", from_address) && Extract(data, "to", to_address) &&
      Extract(data, "amount", amount))
  {
    to_address   = byte_array::FromBase64(to_address);    //  the address needs to be converted
    from_address = byte_array::FromBase64(from_address);  //  the address needs to be converted

    WalletRecord to_record{};
    WalletRecord from_record{};

    if (!GetStateRecord(from_record, from_address))
    {
      return Status::FAILED;
    }

    // check the balance here to limit further reads if required
    if (from_record.balance < amount)
    {
      return Status::FAILED;
    }

    if (!GetOrCreateStateRecord(to_record, to_address))
    {
      return Status::FAILED;
    }

    // update the records
    from_record.balance -= amount;
    to_record.balance += amount;

    // write the records back to the state database
    SetStateRecord(from_record, from_address);
    SetStateRecord(to_record, to_address);
  }

  return Status::OK;
}

Contract::Status TokenContract::Balance(Query const &query, Query &response)
{
  Status status = Status::FAILED;

  ConstByteArray address;
  if (Extract(query, "address", address))
  {
    address = FromBase64(address);

    // lookup the record
    WalletRecord record{};
    GetStateRecord(record, address);

    // formulate the response
    response = Variant::Object();
    response["balance"] = record.balance;

    status = Status::OK;
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Incorrect parameters to balance query");
  }

  return status;
}

}  // namespace ledger
}  // namespace fetch
