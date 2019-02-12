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
#include "ledger/chaincode/deed.hpp"
#include "ledger/chaincode/wallet_record.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include <functional>
#include <memory>
#include <set>
#include <stdexcept>

namespace fetch {
namespace ledger {

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
