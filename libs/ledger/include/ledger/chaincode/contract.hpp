#pragma once
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

#include "variant/variant.hpp"
#include "core/serializers/byte_array_buffer.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/identifier.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

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

class Contract
{
public:
  enum class Status
  {
    OK = 0,
    FAILED,
    NOT_FOUND,
  };

  using ContractName          = chain::TransactionSummary::ContractName;
  using Transaction           = chain::Transaction;
  using Query                 = variant::Variant;
  using TransactionHandler    = std::function<Status(Transaction const &)>;
  using TransactionHandlerMap = std::unordered_map<ContractName, TransactionHandler>;
  using QueryHandler          = std::function<Status(Query const &, Query &)>;
  using QueryHandlerMap       = std::unordered_map<ContractName, QueryHandler>;
  using Counter               = std::atomic<std::size_t>;
  using CounterMap            = std::unordered_map<ContractName, Counter>;
  using StorageInterface      = ledger::StorageInterface;
  using ResourceSet           = chain::TransactionSummary::ResourceSet;

  Contract(Contract const &) = delete;
  Contract(Contract &&)      = delete;
  Contract &operator=(Contract const &) = delete;
  Contract &operator=(Contract &&) = delete;

  Status DispatchQuery(ContractName const &name, Query const &query, Query &response)
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

  Status DispatchTransaction(byte_array::ConstByteArray const &name, Transaction const &tx)
  {
    Status status{Status::NOT_FOUND};

    auto it = transaction_handlers_.find(name);
    if (it != transaction_handlers_.end())
    {

      // lock the contract resources
      LockResources(tx.summary().resources);

      // dispatch the contract
      status = it->second(tx);

      // unlock the contract resources
      UnlockResources(tx.summary().resources);

      ++transaction_counters_[name];
    }

    return status;
  }

  void Attach(StorageInterface &state)
  {
    state_ = &state;
  }

  void Detach()
  {
    state_ = nullptr;
  }

  std::size_t GetQueryCounter(std::string const &name)
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

  std::size_t GetTransactionCounter(std::string const &name)
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

  bool ParseAsJson(Transaction const &tx, variant::Variant &output);

  Identifier const &identifier() const
  {
    return contract_identifier_;
  }

  QueryHandlerMap const &query_handlers() const
  {
    return query_handlers_;
  }

  TransactionHandlerMap const &transaction_handlers() const
  {
    return transaction_handlers_;
  }

  storage::ResourceAddress CreateStateIndex(byte_array::ByteArray const &suffix) const
  {
    byte_array::ByteArray index;
    index.Append(contract_identifier_.name_space(), ".state.", suffix);
    return storage::ResourceAddress{index};
  }

protected:
  explicit Contract(byte_array::ConstByteArray const &identifier)
    : contract_identifier_{identifier}
  {}

  template <typename C>
  void OnTransaction(std::string const &name, C *instance, Status (C::*func)(Transaction const &))
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
  void OnQuery(std::string const &name, C *instance, Status (C::*func)(Query const &, Query &))
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

  StorageInterface &state()
  {
    detailed_assert(state_ != nullptr);
    return *state_;
  }

  template <typename T>
  bool GetOrCreateStateRecord(T &record, byte_array::ByteArray const &address)
  {

    // create the index that is required
    auto index = CreateStateIndex(address);

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
  bool GetStateRecord(T &record, byte_array::ByteArray const &address)
  {

    // create the index that is required
    auto index = CreateStateIndex(address);

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
  void SetStateRecord(T const &record, byte_array::ByteArray const &address)
  {
    auto index = CreateStateIndex(address);

    // serialize the record to the buffer
    serializers::ByteArrayBuffer buffer;
    buffer << record;

    // store the buffer
    state().Set(index, buffer.data());
  }

private:
  bool LockResources(ResourceSet const &resources)
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

  bool UnlockResources(ResourceSet const &resources)
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

  Identifier            contract_identifier_;
  QueryHandlerMap       query_handlers_{};
  TransactionHandlerMap transaction_handlers_{};
  CounterMap            transaction_counters_{};
  CounterMap            query_counters_{};
  StorageInterface *    state_ = nullptr;
};

}  // namespace ledger
}  // namespace fetch
