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

#include <functional>
#include <memory>
#include <set>
#include <stdexcept>

#include "ledger/chaincode/token_contract.hpp"
#include "core/byte_array/decoders.hpp"
#include "crypto/fnv.hpp"
#include "ledger/chaincode/token_contract_deed.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

using fetch::variant::Variant;
using fetch::variant::Extract;
using fetch::byte_array::ConstByteArray;
using fetch::byte_array::FromBase64;

namespace fetch {
namespace ledger {

namespace {

byte_array::ConstByteArray const ADDRESS_NAME{"address"};
byte_array::ConstByteArray const FROM_NAME{"from"};
byte_array::ConstByteArray const TO_NAME{"to"};
byte_array::ConstByteArray const AMOUNT_NAME{"amount"};
byte_array::ConstByteArray const TRANSFER_NAME{"transfer"};
byte_array::ConstByteArray const AMEND_NAME{"amend"};
byte_array::ConstByteArray const THRESHOLDS_NAME{"thresholds"};
byte_array::ConstByteArray const SIGNEES_NAME{"signees"};

using DeedShrdPtr = std::shared_ptr<Deed>;

/**
 * Deserialises deed from Tx JSON data
 *
 * @details The deserialised `deed` data-member can be nullptr (non-initialised
 * `std::smart_ptr<Deed>`) in the case when the objective is to REMOVE the deed
 * (Tx JSON data contain only `address` element, but `signees` and `thresholds`
 * are **NOT** present).
 *
 * @param data Variant type object deserialised from Tx JSON data, which is
 * expected to contain the deed definition. This variant object will be used
 * for deserialisation to `Deed` structure.
 *
 * @return true if deserialisation passed successfully, false otherwise.
 */
bool DeedFromVariant(Variant const &variant_deed, DeedShrdPtr &deed)
{
  auto const num_of_items_in_deed = variant_deed.size();
  if (num_of_items_in_deed == 1 && variant_deed.Has(ADDRESS_NAME))
  {
    // This indicates request to REMOVE the deed (only `address` field has been
    // provided).
    deed.reset();
    return true;
  }
  else if (num_of_items_in_deed != 3)
  {
    // This is INVALID attempt to AMEND the deed. Input deed variant is structurally
    // unsound for amend operation because it does NOT contain exactly 3 expected
    // elements (`address`, `signees` and `thresholds`).
    return false;
  }

  auto const v_thresholds{variant_deed[THRESHOLDS_NAME]};
  if (!v_thresholds.IsObject())
  {
    return false;
  }

  Deed::OperationTresholds thresholds;
  v_thresholds.IterateObject([&thresholds](byte_array::ConstByteArray const &operation,
                                           variant::Variant const &          v_threshold) -> bool {
    thresholds[operation] = v_threshold.As<Deed::Weight>();
    return true;
  });

  auto const v_signees{variant_deed[SIGNEES_NAME]};
  if (!v_signees.IsObject())
  {
    return false;
  }

  Deed::Signees signees;
  v_signees.IterateObject([&signees](byte_array::ConstByteArray const &address,
                                     variant::Variant const &          v_weight) -> bool {
    signees[FromBase64(address)] = v_weight.As<Deed::Weight>();
    return true;
  });

  deed = std::make_shared<Deed>(std::move(signees), std::move(thresholds));
  return true;
}

/* Implements a record to store wallet contents. */
struct WalletRecord
{
  uint64_t              balance{0};
  std::shared_ptr<Deed> deed;

  /**
   * Deserialises deed from Tx JSON data
   *
   * @details The deserialised `deed` data-member can be nullptr (non-initialised
   * `std::smart_ptr<Deed>`) in the case when the objective is to REMOVE the deed
   * (Tx JSON data contain only `address` element, but `signees` and `thresholds`
   * are **NOT** present).
   *
   * @param data Variant type object deserialised from Tx JSON data, which is
   * expected to contain the deed definition. This variant object will be used
   * for deserialisation to `Deed` structure.
   *
   * @return true if deserialisation passed successfully, false otherwise.
   */
  bool CreateDeed(Variant const &data)
  {
    if (!DeedFromVariant(data, deed))
    {
      // Invalid format of deed json data from Tx.
      return false;
    }

    if (!deed)
    {
      // Valid case - the deed is **NOT** present **INTENTIONALLY**.
      return true;
    }

    if (deed->IsSane())
    {
      return true;
    }

    deed.reset();
    return false;
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

  if (Extract(data, ADDRESS_NAME, address) && Extract(data, AMOUNT_NAME, amount))
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

/**
 * Handles transaction related to DEED operation (create, amend, delete).
 *
 * @details The transaction shall carry deed JSON structure in it's data member.
 * The data must have expected JSON structure to make deed valid, please see the
 * examples bellow.
 *
 * Bellow is expected deed JSON structure for CREATION of deed on provided
 * address, or AMENDing the deed currently existing on the provided address.
 * All 3 root elements `address`, `signees` and `thresholds` are MANDATORY,
 * `signees` MUST contain at LEAST one signee, and `thresholds` must contain
 * at LEAST one threshold. Where only known & handled thresholds are "transfer"
 * and "amend". The "amend" threshold is utilized for amending (what includes
 * DELETION) of the pre-existing deed.
 * PLEASE BE MINDFUL, that even if permitted, omission of the "amend" threshold
 * has severe consequences - forever DISABLING possibility to amend the deed!
 * {
 *   "address": "BASE64_ENCODED_DESTINATION_ADDRESS",
 *   "signees" : {
 *     "BASE64_ENCODED_ADDRESS_0" : VOTING_WEIGHT_0 as UNSIGNED_INT,
 *     "BASE64_ENCODED_ADDRESS_1" : VOTING_WEIGHT_1 as UNSIGNED_INT,
 *     ...,
 *     "BASE64_ENCODED_ADDRESS_N" : VOTING_WEIGHT_N as UNSIGNED_INT
 *     },
 *   "thresholds" : {
 *     "transfer" : UNSIGNED_INT,
 *     "amend" : UNSIGNED_INT
 *     }
 * }
 * @example Example:
 * {
 *   "address" : "OTc3NTY1MzAwMTMyMzQzMzM2Ng==",
 *   "signees" : {
 *     "NTAzOTA1MzM4NDM3OTM3MzI5Ng==" : 1,
 *     "OTc3NTY1MzAwMTMyMzQzMzM2Ng==" : 2,
 *     "ODc4MDQ4NDMxNDM1MzY4NDgwNg==" : 3
 *     },
 *   "thresholds" : {
 *     "transfer" : 4,
 *     "amend" : 6
 *     }
 * }
 *
 * Bellow is expected deed JSON structure for DELETION of the deed currently
 * existing on the provided address.
 * Both `signees` AND `thresholds` elements must NOT be present to clearly
 * indicate intention to DELETE the pre-existing deed - if any of these elements
 * are present, amend procedure will fail and so pre-existing deed will remain
 * in effect:
 * {
 *   "address": "BASE64_ENCODED_DESTINATION_ADDRESS"
 * }
 * @example
 * {
 *   "address": "MTM1NTgwMTI1ODkwMzQzNDMyODQ="
 * }
 *
 * @param Transaction carrying deed JSON structure in it's data member.
 *
 * @return Status::OK if deed has been incorporated successfully.
 */
Contract::Status TokenContract::Deed(Transaction const &tx)
{
  Variant data;
  if (!ParseAsJson(tx, data))
  {
    return Status::FAILED;
  }

  ConstByteArray address;
  if (!Extract(data, ADDRESS_NAME, address))
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
  {
    // This is AMEND request(there is current deed in effect).

    // Verify that current transaction has authority to AMEND the deed
    // currently in effect.
    if (!record.deed->Verify(tx, AMEND_NAME))
    {
      return Status::FAILED;
    }

    // Amend (replace) previous deed with new one.
    if (!record.CreateDeed(data))
    {
      return Status::FAILED;
    }
  }
  else
  {
    // This is CREATION (there is NO deed associated with the address yet).

    // NECESSARY & SUFFICIENT CONDITION: Signature of the DESTINATION address
    // MUST be present when NO preceding deed exists.
    if (0 == tx.signatures().count(crypto::Identity{address}))
    {
      return Status::FAILED;
    }

    if (!record.CreateDeed(data))
    {
      return Status::FAILED;
    }
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
  if (!Extract(data, FROM_NAME, from_address) || !Extract(data, TO_NAME, to_address) ||
      !Extract(data, AMOUNT_NAME, amount))
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
  {
    // There is current deed in effect.

    // Verify that current transaction possesses authority to perform the transfer
    if (!from_record.deed->Verify(tx, TRANSFER_NAME))
    {
      return Status::FAILED;
    }
  }
  else
  {
    // There is NO deed associated with the SOURCE address.

    // NECESSARY & SUFFICIENT CONDITION to perform transfer: Signature of the
    // SOURCE address MUST be present when NO preceding deed exists.
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
  if (Extract(query, ADDRESS_NAME, address))
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
