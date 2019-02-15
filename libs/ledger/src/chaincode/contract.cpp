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

#include "ledger/chaincode/contract.hpp"
#include "core/json/document.hpp"

namespace fetch {
namespace ledger {

Contract::Contract(byte_array::ConstByteArray const &identifier)
  : contract_identifier_{identifier}
{}

Contract::Status Contract::DispatchQuery(ContractName const &name, Query const &query,
                                         Query &response)
{
  Status status{Status::NOT_FOUND};

  auto it = query_handlers_.find(name);
  if (it != query_handlers_.end())
  {
    status = it->second(query, response);
    ++query_counters_[name];
  }

  return status;
}

Contract::Status Contract::DispatchTransaction(byte_array::ConstByteArray const &name,
                                               Transaction const &               tx)
{
  Status status{Status::NOT_FOUND};

  auto it = transaction_handlers_.find(name);
  if (it != transaction_handlers_.end())
  {

    // lock the contract resources
    if (!LockResources(tx.summary().resources))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "LockResources failed.");
      return Status::FAILED;
    }

    // dispatch the contract
    status = it->second(tx);

    // unlock the contract resources
    if (!UnlockResources(tx.summary().resources))
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "UnlockResources failed.");
      return Status::FAILED;
    }

    ++transaction_counters_[name];
  }

  return status;
}

void Contract::Attach(StorageInterface &state)
{
  state_ = &state;
}

void Contract::Detach()
{
  state_ = nullptr;
}

std::size_t Contract::GetQueryCounter(std::string const &name)
{
  auto it = query_counters_.find(name);
  if (it != query_counters_.end())
  {
    return it->second;
  }
  else
  {
    return 0;
  }
}

std::size_t Contract::GetTransactionCounter(std::string const &name)
{
  auto it = transaction_counters_.find(name);
  if (it != transaction_counters_.end())
  {
    return it->second;
  }
  else
  {
    return 0;
  }
}

bool Contract::ParseAsJson(Transaction const &tx, variant::Variant &output)
{
  bool success = false;

  json::JSONDocument document;

  try
  {
    // parse the data of the transaction
    document.Parse(tx.data());
    success = true;
  }
  catch (json::JSONParseException &ex)
  {
    // expected
  }

  if (success)
  {
    output = std::move(document.root());
  }

  return success;
}

Identifier const &Contract::identifier() const
{
  return contract_identifier_;
}

Contract::QueryHandlerMap const &Contract::query_handlers() const
{
  return query_handlers_;
}

Contract::TransactionHandlerMap const &Contract::transaction_handlers() const
{
  return transaction_handlers_;
}

storage::ResourceAddress Contract::CreateStateIndex(byte_array::ByteArray const &suffix) const
{
  byte_array::ByteArray index;
  index.Append(contract_identifier_.name_space(), ".state.", suffix);
  return storage::ResourceAddress{index};
}

StorageInterface &Contract::state()
{
  detailed_assert(state_ != nullptr);
  return *state_;
}

bool Contract::LockResources(ResourceSet const &resources)
{
  bool success = true;

  for (auto const &group : resources)
  {
    if (!state().Lock(CreateStateIndex(group)))
    {
      success = false;
    }
  }

  return success;
}

bool Contract::UnlockResources(ResourceSet const &resources)
{
  bool success = true;

  for (auto const &group : resources)
  {
    if (!state().Unlock(CreateStateIndex(group)))
    {
      success = false;
    }
  }

  return success;
}

}  // namespace ledger
}  // namespace fetch
