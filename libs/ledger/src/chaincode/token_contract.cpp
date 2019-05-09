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
#include "ledger/chain/v2/transaction.hpp"

#include <functional>
#include <memory>
#include <set>
#include <stdexcept>
#include <numeric>

namespace fetch {
namespace ledger {

TokenContract::TokenContract()
{
  // TODO(tfr): I think the function CreateWealth should be OnInit?
  OnTransaction("deed", this, &TokenContract::Deed);
  OnTransaction("wealth", this, &TokenContract::CreateWealth);
  OnTransaction("transfer", this, &TokenContract::Transfer);
  OnQuery("balance", this, &TokenContract::Balance);
}

uint64_t TokenContract::GetBalance(v2::Address const &address)
{
  uint64_t balance{0};
  ConstByteArray const encoded_address = address.address().ToHex();

  WalletRecord record{};
  if (GetStateRecord(record, encoded_address))
  {
    balance = record.balance;
  }

  return balance;
}

Contract::Status TokenContract::CreateWealth(v2::Transaction const &tx)
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
    GetStateRecord(record, address);

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
 * examples in deed.cpp
 *
 * @param Transaction carrying deed JSON structure in it's data member.
 *
 * @return Status::OK if deed has been incorporated successfully.
 */
Contract::Status TokenContract::Deed(v2::Transaction const &tx)
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
  GetStateRecord(record, address);

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
    if (!tx.IsSignedByFromAddress())
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

Contract::Status TokenContract::Transfer(v2::Transaction const &tx)
{
  Variant data;
  if (!ParseAsJson(tx, data))
  {
    return Status::FAILED;
  }

  ConstByteArray const from_address = tx.from().address().ToHex();
  WalletRecord from_record{};

  if (!GetStateRecord(from_record, from_address))
  {
    return Status::FAILED;
  }

  // determine if the from address does indeed have all the required balance present
  uint64_t total_amount{0};
  for (auto const &transfer : tx.transfers())
  {
    total_amount += transfer.amount;
  }

  // check the balance here to limit further reads if required
  if (from_record.balance < total_amount)
  {
    return Status::FAILED;
  }

  for (auto const &transfer : tx.transfers())
  {
    ConstByteArray const to_address = transfer.to.address().ToHex();
    WalletRecord to_record{};

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
      if (!tx.IsSignedByFromAddress())
      {
        return Status::FAILED;
      }
    }

    // get the state record for the target address
    GetStateRecord(to_record, to_address);

    // update the records
    from_record.balance -= transfer.amount;
    to_record.balance += transfer.amount;

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
