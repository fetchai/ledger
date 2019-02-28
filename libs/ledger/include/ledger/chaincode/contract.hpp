#pragma once
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

#include "core/serializers/byte_array_buffer.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/identifier.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "variant/variant.hpp"

#include <atomic>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace fetch {
namespace variant {
class Variant;
}
namespace ledger {

inline storage::ResourceAddress CreateStateIndexWrapped(byte_array::ByteArray const &global_id,
                                                        byte_array::ByteArray const &local_id,
                                                        byte_array::ByteArray const &suffix)
{
  byte_array::ByteArray index;
  index.Append(global_id, local_id, ".state.", suffix);
  return storage::ResourceAddress{index};
}

class Contract
{
public:
  enum class Status
  {
    OK = 0,
    FAILED,
    NOT_FOUND,
  };

  using ContractName          = TransactionSummary::ContractName;
  using Query                 = variant::Variant;
  using TransactionHandler    = std::function<Status(Transaction const &)>;
  using TransactionHandlerMap = std::unordered_map<ContractName, TransactionHandler>;
  using QueryHandler          = std::function<Status(Query const &, Query &)>;
  using QueryHandlerMap       = std::unordered_map<ContractName, QueryHandler>;
  using Counter               = std::atomic<std::size_t>;
  using CounterMap            = std::unordered_map<ContractName, Counter>;
  using StorageInterface      = ledger::StorageInterface;
  using ResourceSet           = TransactionSummary::ResourceSet;
  using ContractHashes        = TransactionSummary::ContractHashes;

  Contract()                 = default;
  Contract(Contract const &) = delete;
  Contract(Contract &&)      = delete;
  Contract &operator=(Contract const &) = delete;
  Contract &operator=(Contract &&) = delete;
  virtual ~Contract()              = default;

  static constexpr char const *LOGGING_NAME = "Contract";

  Status DispatchQuery(ContractName const &name, Query const &query, Query &response);
  Status DispatchTransaction(byte_array::ConstByteArray const &name, Transaction const &tx,
                             std::vector<std::string> *output_strings);

  void Attach(StorageInterface &state);
  void Detach();

  std::size_t GetQueryCounter(std::string const &name);
  std::size_t GetTransactionCounter(std::string const &name);

  bool ParseAsJson(Transaction const &tx, variant::Variant &output);

  Identifier const &           identifier() const;
  QueryHandlerMap const &      query_handlers() const;
  TransactionHandlerMap const &transaction_handlers() const;

  virtual bool                    SetupHandlers();
  void                            SetNoWriteBack();
  std::vector<std::string> const &PrintStrings();

protected:
  explicit Contract(byte_array::ConstByteArray const &identifier);

  template <typename C>
  void OnTransaction(std::string const &name, C *instance, Status (C::*func)(Transaction const &));
  template <typename C>
  void OnQuery(std::string const &name, C *instance, Status (C::*func)(Query const &, Query &));

  StorageInterface &state();

  template <typename T>
  bool GetOrCreateStateRecord(T &record, byte_array::ByteArray const &address);

  bool StateRecordExists(byte_array::ByteArray const &address);
  template <typename T>
  bool GetStateRecord(T &record, byte_array::ByteArray const &address);
  template <typename T>
  void SetStateRecord(T const &record, byte_array::ByteArray const &address);

  bool CheckRawState(byte_array::ByteArray const &address);
  void SetRawState(byte_array::ByteArray const &payload, byte_array::ByteArray const &address);
  bool GetRawState(byte_array::ByteArray &payload, byte_array::ByteArray const &address);

  std::vector<std::string> print_strings_;
  bool                     allow_write_back_ = true;

private:
  Identifier            contract_identifier_;
  QueryHandlerMap       query_handlers_{};
  TransactionHandlerMap transaction_handlers_{};
  CounterMap            transaction_counters_{};
  CounterMap            query_counters_{};
  StorageInterface *    state_ = nullptr;
};

template <typename C>
void Contract::OnTransaction(std::string const &name, C *instance,
                             Status (C::*func)(Transaction const &))
{
  if (transaction_handlers_.find(name) == transaction_handlers_.end())
  {
    transaction_handlers_[name] = [instance, func](Transaction const &tx) {
      return (instance->*func)(tx);
    };
    transaction_counters_[name] = 0;
  }
  else
  {
    throw std::logic_error("Duplicate transaction handler registered");
  }
}

template <typename C>
void Contract::OnQuery(std::string const &name, C *instance,
                       Status (C::*func)(Query const &, Query &))
{
  if (query_handlers_.find(name) == query_handlers_.end())
  {
    query_handlers_[name] = [instance, func](Query const &query, Query &response) {
      return (instance->*func)(query, response);
    };
    query_counters_[name] = 0;
  }
  else
  {
    throw std::logic_error("Duplicate query handler registered");
  }
}

template <typename T>
bool Contract::GetOrCreateStateRecord(T &record, byte_array::ByteArray const &address)
{
  // create the index that is required
  auto index = CreateStateIndexWrapped(contract_identifier_[0], contract_identifier_[1], address);

  // retrieve the state data
  auto document = state().GetOrCreate(index);
  if (document.failed)
  {
    return false;
  }

  // update the document if it wasn't created
  if (!document.was_created)
  {
    serializers::ByteArrayBuffer buffer(document.document);
    buffer >> record;
  }

  return true;
}

template <typename T>
bool Contract::GetStateRecord(T &record, byte_array::ByteArray const &address)
{

  // create the index that is required
  auto index = CreateStateIndexWrapped(contract_identifier_[0], contract_identifier_[1], address);

  // retrieve the state data
  auto document = state().Get(index);
  if (document.failed)
  {
    return false;
  }

  // update the document if it wasn't created
  serializers::ByteArrayBuffer buffer(document.document);
  buffer >> record;

  return true;
}

template <typename T>
void Contract::SetStateRecord(T const &record, byte_array::ByteArray const &address)
{
  auto index = CreateStateIndexWrapped(contract_identifier_[0], contract_identifier_[1], address);

  // serialize the record to the buffer
  serializers::ByteArrayBuffer buffer;
  buffer << record;

  // store the buffer
  state().Set(index, buffer.data());
}

}  // namespace ledger
}  // namespace fetch
