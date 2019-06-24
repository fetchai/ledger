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
#include "ledger/chain/transaction.hpp"
#include "ledger/chaincode/deed.hpp"
#include "ledger/chaincode/wallet_record.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include <memory>
#include <unordered_map>

namespace fetch {
namespace ledger {
namespace {

bool IsOperationValid(WalletRecord const &record, Transaction const &tx, ConstByteArray const &operation)
{
  // perform validation checks
  if (record.deed)
  {
    // There is current deed in effect.

    // Verify that current transaction possesses authority to perform the transfer
    if (!record.deed->Verify(tx, operation))
    {
      return false;
    }
  }
  else
  {
    // There is NO deed associated with the SOURCE address.

    // NECESSARY & SUFFICIENT CONDITION to perform transfer: Signature of the
    // SOURCE address MUST be present when NO preceding deed exists.
    if (!tx.IsSignedByFromAddress())
    {
      return false;
    }
  }

  return true;
}

constexpr uint64_t MAX_TOKENS           = 0xFFFFFFFFFFFFFFFFull;
constexpr uint64_t STAKE_WARM_UP_PERIOD = 5;

} // namespace


TokenContract::TokenContract()
{
  // TODO(tfr): I think the function CreateWealth should be OnInit?
  OnTransaction("deed", this, &TokenContract::Deed);
  OnTransaction("wealth", this, &TokenContract::CreateWealth);
  OnTransaction("transfer", this, &TokenContract::Transfer);
  OnTransaction("addStake", this, &TokenContract::AddStake);
  OnQuery("balance", this, &TokenContract::Balance);
  OnQuery("stake", this, &TokenContract::Stake);
}

uint64_t TokenContract::GetBalance(Address const &address)
{
  WalletRecord record{};
  GetStateRecord(record, address.display());

  return record.balance;
}

bool TokenContract::AddTokens(Address const &address, uint64_t amount)
{
  WalletRecord record{};
  GetStateRecord(record, address.display());

  if (amount > (MAX_TOKENS - record.balance))
  {
    return false;
  }

  record.balance += amount;

  SetStateRecord(record, address.display());

  return true;
}

bool TokenContract::SubtractTokens(Address const &address, uint64_t amount)
{
  WalletRecord record{};
  GetStateRecord(record, address.display());

  if (amount > record.balance)
  {
    return false;
  }

  record.balance -= amount;

  SetStateRecord(record, address.display());

  return true;
}

bool TokenContract::TransferTokens(Transaction const &tx, Address const &to, uint64_t amount)
{
  // look up the state record (to see if there is a deed associated with this address)
  WalletRecord from_record{};
  if (!GetStateRecord(from_record, tx.from().display()))
  {
    return false;
  }

  // perform validation checks
  if (from_record.deed)
  {
    // There is current deed in effect.

    // Verify that current transaction possesses authority to perform the transfer
    if (!from_record.deed->Verify(tx, TRANSFER_NAME))
    {
      return false;
    }
  }
  else
  {
    // There is NO deed associated with the SOURCE address.

    // NECESSARY & SUFFICIENT CONDITION to perform transfer: Signature of the
    // SOURCE address MUST be present when NO preceding deed exists.
    if (!tx.IsSignedByFromAddress())
    {
      return false;
    }
  }

  return SubtractTokens(tx.from(), amount) && AddTokens(to, amount);
}

Contract::Status TokenContract::CreateWealth(Transaction const &tx, BlockIndex)
{
  Contract::Status status{Contract::Status::FAILED};

  // parse the payload as JSON
  Variant data;
  if (ParseAsJson(tx, data))
  {
    // attempt to extract the amount field
    uint64_t amount{0};
    if (Extract(data, AMOUNT_NAME, amount))
    {
      // mint new tokens
      if (AddTokens(tx.from(), amount))
      {
        status = Contract::Status::OK;
      }
    }
  }

  return status;
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
Contract::Status TokenContract::Deed(Transaction const &tx, BlockIndex)
{
  Variant data;
  if (!ParseAsJson(tx, data))
  {
    return Status::FAILED;
  }

  // retrieve the record (if it exists)
  WalletRecord record{};
  GetStateRecord(record, tx.from().display());

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

  SetStateRecord(record, tx.from().display());

  return Status::OK;
}

Contract::Status TokenContract::Transfer(Transaction const &tx, BlockIndex)
{
  FETCH_UNUSED(tx);
  return Status::FAILED;
}

Contract::Status TokenContract::AddStake(Transaction const &tx, BlockIndex block)
{
  Status status{Status::FAILED};

  // parse the payload as JSON
  Variant data;
  if (ParseAsJson(tx, data))
  {
    // attempt to extract the amount field
    uint64_t amount{0};
    if (Extract(data, AMOUNT_NAME, amount))
    {
      // look up the state record (to see if there is a deed associated with this address)
      WalletRecord record{};
      if (GetStateRecord(record, tx.from().display()))
      {
        // ensure the operation is permitted
        if (IsOperationValid(record, tx, STAKE_NAME))
        {
          // stake the amount
          if (record.balance >= amount)
          {
            record.balance -= amount;
            record.stake += amount;

            // record the stake update event
            stake_updates_.emplace_back(StakeUpdate{tx.from(), block + STAKE_WARM_UP_PERIOD, amount});

            // save the state
            SetStateRecord(record, tx.from().display());

            status = Status::OK;
          }
        }
      }
    }
  }

  return status;
}

Contract::Status TokenContract::Balance(Query const &query, Query &response)
{
  Status status = Status::FAILED;

  ConstByteArray input;
  if (Extract(query, ADDRESS_NAME, input))
  {
    // attempt to parse the input address
    Address address{};
    if (Address::Parse(input, address))
    {
      // lookup the record
      WalletRecord record{};
      GetStateRecord(record, address.display());

      // formulate the response
      response            = Variant::Object();
      response["balance"] = record.balance;

      status = Status::OK;
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Incorrect parameters to balance query");
  }

  return status;
}

Contract::Status TokenContract::Stake(Query const &query, Query &response)
{
  Status status = Status::FAILED;

  ConstByteArray input;
  if (Extract(query, ADDRESS_NAME, input))
  {
    // attempt to parse the input address
    Address address{};
    if (Address::Parse(input, address))
    {
      // lookup the record
      WalletRecord record{};
      GetStateRecord(record, address.display());

      // formulate the response
      response          = Variant::Object();
      response["stake"] = record.stake;

      status = Status::OK;
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Incorrect parameters to balance query");
  }

  return status;
}

}  // namespace ledger
}  // namespace fetch
