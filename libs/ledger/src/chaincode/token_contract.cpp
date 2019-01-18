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

#include "ledger/chaincode/token_contract.hpp"
#include "core/byte_array/decoders.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chaincode/token_contract_deed.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include <functional>
#include <memory>
#include <set>
#include <stdexcept>

using fetch::variant::Variant;
using fetch::variant::Extract;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;

namespace fetch {
namespace ledger {

namespace {

byte_array::ConstByteArray const address_name{"address"};
byte_array::ConstByteArray const from_name{"from"};
byte_array::ConstByteArray const to_name{"to"};
byte_array::ConstByteArray const amount_name{"amount"};
byte_array::ConstByteArray const transfer_name{"transfer"};
byte_array::ConstByteArray const modify_name{"modify"};
byte_array::ConstByteArray const thresholds_name{"thresholds"};
byte_array::ConstByteArray const signees_name{"signees"};

using DeedShrdPtr = std::shared_ptr<Deed>;

DeedShrdPtr DeedFromVariant(Variant const &variant_deed)
{
  Variant v_thresholds;
  if (!Extract(variant_deed, thresholds_name, v_thresholds))
  {
    return DeedShrdPtr{};
  }

  Deed::OperationTresholds thresholds;
  v_thresholds.IterateObject([&thresholds](byte_array::ConstByteArray const &operation,
                                           variant::Variant const &          v_threshold) -> bool {
    thresholds[operation] = v_threshold.As<Weight>();
    return true;
  });

  Variant v_signees;
  if (!Extract(variant_deed, signees_name, v_signees))
  {
    return DeedShrdPtr{};
  }

  Deed::Signees signees;
  v_signees.IterateObject([&signees](byte_array::ConstByteArray const &address,
                                     variant::Variant const &          v_weight) -> bool {
    signees[FromBase64(address)] = v_weight.As<Weight>();
    return true;
  });

  return std::make_shared<Deed>(std::move(signees), std::move(thresholds));
}

/* Implements a record to store wallet contents. */
struct WalletRecord
{
  uint64_t              balance{0};
  std::shared_ptr<Deed> deed;

  void CreateDeed(Variant const &data)
  {
    deed = DeedFromVariant(data);
    if (!deed->IsSane())
    {
      deed.reset();
    }
  }

  template <typename T>
  friend void Serialize(T &serializer, WalletRecord const &b)
  {
    if (b.deed)
    {
      serializer.Append(b.balance, true, *b.deed);
    }
    else
    {
      serializer.Append(b.balance, false);
    }
  }

  template <typename T>
  friend void Deserialize(T &serializer, WalletRecord &b)
  {
    bool has_deed = false;
    serializer >> b.balance >> has_deed;
    if (has_deed)
    {
      if (!b.deed)
      {
        b.deed.reset(new Deed{});
      }
      serializer >> *b.deed;
    }
  }
};

}  // namespace

TokenContract::TokenContract()
  : Contract("fetch.token")
{
  // TODO(tfr): I think the function CreateWealth should be OnInit?
  OnTransaction("deed", this, &TokenContract::Deed);
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
  uint64_t       amount{0};

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

Contract::Status TokenContract::Deed(Transaction const &tx)
{
  Variant data;
  if (!ParseAsJson(tx, data))
  {
    return Status::FAILED;
  }

  ConstByteArray address;
  if (!Extract(data, address_name, address))
  {
    return Status::FAILED;
  }
  address = FromBase64(address);  //  the address needs to be converted

  // retrieve the record (if it exists)
  WalletRecord record{};
  if (!GetOrCreateStateRecord(record, address))
  {
    return Status::FAILED;
  }

  if (record.deed)
  {  // This is MODIFICATION (there is current deed in effect)
    // Verify if current transaction has authority to MODIFY the deed
    // currently in effect.
    if (record.deed->Verify(tx, modify_name))
    {
      // Replace previous deed with new one.
      record.CreateDeed(data);
    }
    else
    {
      return Status::FAILED;
    }
  }
  else
  {  // This is CREATION (there is NO deed associated with the address yet)
    // NECESSARY & SUFFICIENT CONDITION: Signature of destination address
    // MUST be present when NO preceeding deed exists.
    if (0 == tx.signatures().count(crypto::Identity{address}))
    {
      return Status::FAILED;
    }
    record.CreateDeed(data);
  }

  if (!record.deed)
  {
    return Status::FAILED;
  }

  SetStateRecord(record, address);

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
  if (!Extract(data, "from", from_address) || !Extract(data, "to", to_address) ||
      !Extract(data, "amount", amount))
  {
    return Status::FAILED;
  }

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

  if (from_record.deed)
  {  // There is current deed in effect:
    // Verify if current transaction has authority to perform the transfer
    if (!from_record.deed->Verify(tx, transfer_name))
    {
      return Status::FAILED;
    }
  }
  else
  {  // There is NO deed associated with the source address
    // NECESSARY & SUFFICIENT CONDITION to perform transfer: Signature of
    // destination address MUST be present when NO preceeding deed exists.
    if (0 == tx.signatures().count(crypto::Identity{from_address}))
    {
      return Status::FAILED;
    }
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

  return Status::OK;
}

Contract::Status TokenContract::Balance(Query const &query, Query &response)
{
  Status status = Status::FAILED;

  ConstByteArray address;
  if (Extract(query, address_name, address))
  {
    address = FromBase64(address);

    // lookup the record
    WalletRecord record{};
    GetStateRecord(record, address);

    // formulate the response
    response            = Variant::Object();
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
