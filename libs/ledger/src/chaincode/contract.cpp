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
#include "core/byte_array/decoders.hpp"
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

// Contracts can override this to setup handlers from source
bool Contract::SetupHandlers()
{
  return true;
}

Contract::Status Contract::DispatchTransaction(byte_array::ConstByteArray const &name,
                                               Transaction const &               tx,
                                               std::vector<std::string> *output_strings = nullptr)
{
  Status status{Status::NOT_FOUND};

  auto it = transaction_handlers_.find(name);
  if (it != transaction_handlers_.end())
  {
    // dispatch the contract
    status = it->second(tx);
    ++transaction_counters_[name];
  }

  if (output_strings)
  {
    (*output_strings) = print_strings_;
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
    // TODO(HUT): this can't be good for performance
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

StorageInterface &Contract::state()
{
  detailed_assert(state_ != nullptr);
  return *state_;
}

bool Contract::CheckRawState(byte_array::ByteArray const &address)
{
  auto document = state().Get(storage::ResourceAddress{address});

  return !document.failed;
}

void Contract::SetRawState(byte_array::ByteArray const &payload,
                           byte_array::ByteArray const &address)
{
  state().Set(storage::ResourceAddress{address}, payload);
}

bool Contract::GetRawState(byte_array::ByteArray &payload, byte_array::ByteArray const &address)
{
  auto document = state().Get(storage::ResourceAddress{address});

  if (document.failed)
  {
    return false;
  }

  // update the document if it wasn't created
  serializers::ByteArrayBuffer buffer(document.document);
  payload = buffer.data();

  return true;
}

bool Contract::StateRecordExists(byte_array::ByteArray const &address)
{
  // create the index that is required
  auto index = CreateStateIndexWrapped(contract_identifier_[0], contract_identifier_[1], address);

  // retrieve the state data
  auto document = state().Get(index);
  if (document.failed)
  {
    return false;
  }

  return true;
}

void Contract::SetNoWriteBack()
{
  allow_write_back_ = false;
}

std::vector<std::string> const &Contract::PrintStrings()
{
  return print_strings_;
}

}  // namespace ledger
}  // namespace fetch
