//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "chain/constants.hpp"
#include "chain/transaction.hpp"
#include "core/byte_array/decoders.hpp"
#include "crypto/fnv.hpp"
#include "crypto/identity.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/chaincode/deed.hpp"
#include "ledger/chaincode/token_contract.hpp"
#include "ledger/chaincode/wallet_record.hpp"
#include "logging/logging.hpp"
#include "variant/variant.hpp"
#include "variant/variant_utils.hpp"

#include <memory>
#include <sstream>
#include <unordered_map>

namespace fetch {
namespace ledger {
namespace {

using fetch::variant::Variant;

template <typename T>
std::string ConvertToString(T const &value)
{
  std::ostringstream oss;
  oss << value;

  return oss.str();
}

bool IsOperationValid(WalletRecord const &record, chain::Transaction const &tx,
                      Deed::Operation const &operation)
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

constexpr uint64_t MAX_TOKENS = 0xFFFFFFFFFFFFFFFFull;

}  // namespace

TokenContract::TokenContract()
{
  OnTransaction("deed", this, &TokenContract::UpdateDeed);
  OnTransaction("transfer", this, &TokenContract::Transfer);
  OnTransaction("addStake", this, &TokenContract::AddStake);
  OnTransaction("deStake", this, &TokenContract::DeStake);
  OnTransaction("collectStake", this, &TokenContract::CollectStake);
  OnQuery("balance", this, &TokenContract::Balance);
  OnQuery("queryDeed", this, &TokenContract::QueryDeed);
  OnQuery("stake", this, &TokenContract::Stake);
  OnQuery("cooldownStake", this, &TokenContract::CooldownStake);
}

DeedPtr TokenContract::GetDeed(chain::Address const &address)
{
  DeedPtr deed{};

  WalletRecord record{};
  if (GetStateRecord(record, address))
  {
    deed = record.deed;
  }

  return deed;
}

void TokenContract::SetDeed(chain::Address const &address, DeedPtr const &deed)
{
  // create or lookup the wallet record
  WalletRecord record{};
  GetStateRecord(record, address);

  // update the deed
  record.deed = deed;

  // write back the wallet record
  SetStateRecord(record, address);
}

uint64_t TokenContract::GetBalance(chain::Address const &address)
{
  WalletRecord record{};
  GetStateRecord(record, address);

  return record.balance;
}

bool TokenContract::AddTokens(chain::Address const &address, uint64_t amount)
{
  WalletRecord record{};
  GetStateRecord(record, address);

  if (amount > (MAX_TOKENS - record.balance))
  {
    return false;
  }

  record.balance += amount;

  auto const status = SetStateRecord(record, address);

  return status == StateAdapter::Status::OK;
}

bool TokenContract::SubtractTokens(chain::Address const &address, uint64_t amount)
{
  WalletRecord record{};
  GetStateRecord(record, address);

  if (amount > record.balance)
  {
    return false;
  }

  record.balance -= amount;

  auto const status = SetStateRecord(record, address);

  return status == StateAdapter::Status::OK;
}

bool TokenContract::TransferTokens(chain::Transaction const &tx, chain::Address const &to,
                                   uint64_t amount)
{
  // look up the state record (to see if there is a deed associated with this address)
  WalletRecord from_record{};
  if (!GetStateRecord(from_record, tx.from()))
  {
    return false;
  }

  // perform validation checks
  if (from_record.deed)
  {
    // There is current deed in effect.

    // Verify that current transaction possesses authority to perform the transfer
    if (!from_record.deed->Verify(tx, Deed::TRANSFER))
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
Contract::Result TokenContract::UpdateDeed(chain::Transaction const &tx)
{
  Variant data;
  if (!ParseAsJson(tx, data))
  {
    return {Status::FAILED};
  }

  // retrieve the record (if it exists)
  WalletRecord record{};
  GetStateRecord(record, tx.from());

  if (record.deed)
  {
    // This is AMEND request(there is current deed in effect).

    // Verify that current transaction has authority to AMEND the deed
    // currently in effect.
    if (!record.deed->Verify(tx, Deed::AMEND))
    {
      return {Status::FAILED};
    }

    // Amend (replace) previous deed with new one.
    if (!record.CreateDeed(data))
    {
      return {Status::FAILED};
    }
  }
  else
  {
    // This is CREATION (there is NO deed associated with the address yet).

    // NECESSARY & SUFFICIENT CONDITION: Signature of the DESTINATION address
    // MUST be present when NO preceding deed exists.
    if (!tx.IsSignedByFromAddress())
    {
      return {Status::FAILED};
    }

    if (!record.CreateDeed(data))
    {
      return {Status::FAILED};
    }
  }

  auto const status = SetStateRecord(record, tx.from());
  if (status != StateAdapter::Status::OK)
  {
    return {Status::FAILED};
  }

  return {Status::OK};
}

Contract::Result TokenContract::Transfer(chain::Transaction const &tx)
{
  FETCH_UNUSED(tx);
  return {Status::FAILED};
}

Contract::Result TokenContract::AddStake(chain::Transaction const &tx)
{
  FETCH_LOG_INFO(LOGGING_NAME, "Adding stake!");

  // parse the payload as JSON
  Variant data;
  if (ParseAsJson(tx, data))
  {
    // attempt to extract the amount field
    uint64_t       amount{0};
    ConstByteArray input;

    if (Extract(data, AMOUNT_NAME, amount) && Extract(data, ADDRESS_NAME, input))
    {
      // look up the state record (to see if there is a deed associated with this address)
      WalletRecord record{};
      if (GetStateRecord(record, tx.from()))
      {
        // ensure the transaction has authority over this deed
        if (IsOperationValid(record, tx, Deed::STAKE))
        {
          // stake the amount
          if (record.balance >= amount)
          {
            record.balance -= amount;
            record.stake += amount;

            FETCH_LOG_INFO(LOGGING_NAME, "Stake updates are happening");

            // record the stake update event
            stake_updates_.emplace_back(
                StakeUpdateEvent{context().block_index + chain::STAKE_WARM_UP_PERIOD,
                                 crypto::Identity(input.FromBase64()), amount});

            // save the state
            auto const status = SetStateRecord(record, tx.from());
            if (status == StateAdapter::Status::OK)
            {
              return {Status::OK};
            }
          }
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "Attempted staking operation was invalid!");
        }
      }
    }
  }

  return {Status::FAILED};
}

Contract::Result TokenContract::DeStake(chain::Transaction const &tx)
{
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
      if (GetStateRecord(record, tx.from()))
      {
        // ensure the transaction has authority over this deed
        if (IsOperationValid(record, tx, Deed::STAKE))
        {
          FETCH_LOG_INFO(LOGGING_NAME, "Destaking! : ", amount, " of ", record.stake);
          // destake the amount
          if (record.stake >= amount)
          {
            record.stake -= amount;

            // Put it in a cooldown state
            record.cooldown_stake[context().block_index + chain::STAKE_COOL_DOWN_PERIOD] += amount;

            // save the state
            auto const status = SetStateRecord(record, tx.from());
            if (status == StateAdapter::Status::OK)
            {
              return {Status::OK};
            }
          }
        }
      }
    }
  }

  return {Status::FAILED};
}

Contract::Result TokenContract::CollectStake(chain::Transaction const &tx)
{
  WalletRecord record{};

  if (GetStateRecord(record, tx.from()))
  {
    // ensure the transaction has authority over this deed
    if (IsOperationValid(record, tx, Deed::STAKE))
    {
      // Collect all cooled down stakes and put them back into the account
      record.CollectStake(context().block_index);

      auto const status = SetStateRecord(record, tx.from());
      if (status == StateAdapter::Status::OK)
      {
        return {Status::OK};
      }
    }
  }

  return {Status::FAILED};
}

Contract::Status TokenContract::Balance(Query const &query, Query &response)
{
  ConstByteArray input;
  if (Extract(query, ADDRESS_NAME, input))
  {
    // attempt to parse the input address
    chain::Address address{};
    if (chain::Address::Parse(input, address))
    {
      // look up the record
      WalletRecord record{};
      GetStateRecord(record, address);

      // formulate the response
      response            = Variant::Object();
      response["balance"] = ConvertToString(record.balance);

      return Status::OK;
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Incorrect parameters to balance query");
  }

  return Status::FAILED;
}

Contract::Status TokenContract::QueryDeed(Query const &query, Query &response)
{
  ConstByteArray input;
  if (Extract(query, ADDRESS_NAME, input))
  {
    // attempt to parse the input address
    chain::Address address{};
    if (chain::Address::Parse(input, address))
    {
      // look up the record
      WalletRecord record{};
      GetStateRecord(record, address);

      // formulate the response
      response = record.ExtractDeed();
      return Status::OK;
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Incorrect parameters to deed query");
  }

  return Status::FAILED;
}

Contract::Status TokenContract::Stake(Query const &query, Query &response)
{
  ConstByteArray input;
  if (Extract(query, ADDRESS_NAME, input))
  {
    // attempt to parse the input address
    chain::Address address{};
    if (chain::Address::Parse(input, address))
    {
      // look up the record
      WalletRecord record{};
      GetStateRecord(record, address);

      // formulate the response
      response          = Variant::Object();
      response["stake"] = ConvertToString(record.stake);

      return Status::OK;
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Incorrect parameters to stake query");
  }

  return Status::FAILED;
}

Contract::Status TokenContract::CooldownStake(Query const &query, Query &response)
{
  ConstByteArray input;
  if (Extract(query, ADDRESS_NAME, input))
  {
    // attempt to parse the input address
    chain::Address address{};
    if (chain::Address::Parse(input, address))
    {
      // look up the record
      WalletRecord record{};
      GetStateRecord(record, address);

      // formulate the response
      response  = Variant::Object();
      auto keys = Variant::Object();

      for (auto const &it : record.cooldown_stake)
      {
        keys[std::to_string(it.first)] = it.second;
      }

      response["cooldownStake"] = keys;

      return Status::OK;
    }
  }
  else
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Incorrect parameters to cooldown stake query");
  }

  return Status::FAILED;
}

void TokenContract::ClearStakeUpdates()
{
  stake_updates_.clear();
}

void TokenContract::ExtractStakeUpdates(StakeUpdateEvents &updates)
{
  updates = std::move(stake_updates_);
}

}  // namespace ledger
}  // namespace fetch
