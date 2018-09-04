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

#include "core/script/variant.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/identifier.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "vm/module.hpp"

#include <atomic>
#include <functional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace fetch {
namespace script {
class Variant;
}
namespace ledger {

template <typename S>
std::unique_ptr<vm::Module> CreateVMDefinition(S *sc = nullptr);


class Contract
{
public:
  enum class Status
  {
    OK = 0,
    FAILED,
    NOT_FOUND,
  };

  using transaction_type             = chain::Transaction;
  using query_type                   = script::Variant;
  using transaction_handler_type     = std::function<Status(transaction_type const &)>;
  using transaction_handler_map_type = std::unordered_map<std::string, transaction_handler_type>;
  using query_handler_type           = std::function<Status(query_type const &, query_type &)>;
  using query_handler_map_type       = std::unordered_map<std::string, query_handler_type>;
  using counter_type                 = std::atomic<std::size_t>;
  using counter_map_type             = std::unordered_map<std::string, counter_type>;
  using storage_type                 = ledger::StorageInterface;
  using resource_set_type            = chain::TransactionSummary::resource_set_type;

  Contract(Contract const &) = delete;
  Contract(Contract &&)      = delete;
  Contract &operator=(Contract const &) = delete;
  Contract &operator=(Contract &&) = delete;

  /* Dispatches a query to the contract.
   *
   * A query is by default not allowed to modify the state.
   *
   * @param name is the query name.
   * @param query is the actual query.
   * @param response is a reference to a response object.
   */
  Status DispatchQuery(std::string const &name, query_type const &query, query_type &response)
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

  /* Executes a transaction.
   *
   * @param name is the name of the contract method.
   * @param tx is the associated transaction.
   */
  Status DispatchTransaction(std::string const &name, transaction_type const &tx)
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

  /* Attaches a storage unit.
   *
   * @param state is a storage unit that implements a storage interface.
   */
  void Attach(storage_type &state) { state_ = &state; }

  /* Detaches the storage unit.
   *
   */
  void Detach() { state_ = nullptr; }

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

  bool ParseAsJson(transaction_type const &tx, script::Variant &output);

  Identifier const &identifier() const { return contract_identifier_; }

  query_handler_map_type const &query_handlers() const { return query_handlers_; }

  transaction_handler_map_type const &transaction_handlers() const { return transaction_handlers_; }

  storage::ResourceAddress CreateStateIndex(byte_array::ByteArray const &suffix) const
  {
    byte_array::ByteArray index(contract_identifier_.name_space());
    index = index + ".state." + suffix;
    return storage::ResourceAddress{index};
  }


  template <typename S>
  friend  std::unique_ptr<vm::Module> CreateVMDefinition(S *);

protected:
  explicit Contract(std::string const &identifer) : contract_identifier_{identifer} {}

  /* Method to expose a given contraction function.
   *
   * @param name is the name of transaction
   * @param instance is a pointer to the class instance holding the contract.
   * @param func is a member function of instance thtat takes a transaction as argument.
   */
  template <typename C>
  void OnTransaction(std::string const &name, C *instance,
                     Status (C::*func)(transaction_type const &))
  {
    if (transaction_handlers_.find(name) == transaction_handlers_.end())
    {
      transaction_handlers_[name] = [instance, func](transaction_type const &tx) {
        return (instance->*func)(tx);
      };
      transaction_counters_[name] = 0;
    }
    else
    {
      throw std::logic_error("Duplicate transaction handler registered");
    }
  }

  /* Method to expose a given query function.
   *
   * @param name is the name of transaction
   * @param instance is a pointer to the class instance holding the contract.
   * @param func is a member function of instance thtat takes a transaction as argument.
   */
  template <typename C>
  void OnQuery(std::string const &name, C *instance,
               Status (C::*func)(query_type const &, query_type &))
  {
    if (query_handlers_.find(name) == query_handlers_.end())
    {
      query_handlers_[name] = [instance, func](query_type const &query, query_type &response) {
        return (instance->*func)(query, response);
      };
      query_counters_[name] = 0;
    }
    else
    {
      throw std::logic_error("Duplicate query handler registered");
    }
  }

  // TODO(tfr): We may want to have follow
  // - OnInit   - Called when the contract is initiated.
  // - OnBlock  - Called when a new block is mined.

  // TODO(tfr): The helper functions below should be careful designed such
  // the contract cannot go outside of its scope.

  /* Helper function get or create a record from the storage unit.
   *
   * @param record is a record that needs to be read from the state db.
   * @param address is the address where the record is located.
   */
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

  /* Helper function to get a state record.
   *
   * @param record is a record that needs to be read from the state db.
   * @param address is the address where the record is located.
   */
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

  /* Helper function to set a state record.
   *
   * @param record is a record that needs to be written from the state db.
   * @param address is the address where the record should be located.
   */
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
  /* Returns a reference to the storage unit. */
  storage_type &state()
  {
    // Note this function should be privately inherited such that contracts can only access
    // the storage units through the safe helper functions.
    detailed_assert(state_ != nullptr);
    return *state_;
  }

  /* Locks resources needed by a contract.
   * 
   * This method is used to ensure that no other contract will have access to the resource
   * for the duration of the contract execution.
   */
  bool LockResources(resource_set_type const &resources)
  {
    bool success = true;

    for (auto const &group : resources)
    {
      if (!state().Lock(CreateStateIndex(group)))
      {

        // TODO(tfr): Do we have a mechanism for recovering when this happens?
        success = false;
      }
    }

    return success;
  }

  /* Releases the lock on a given set of resources.
   * 
   * This method is used to release the locks after contract execution.
   */
  bool UnlockResources(resource_set_type const &resources)
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

  Identifier                   contract_identifier_;
  query_handler_map_type       query_handlers_{};
  transaction_handler_map_type transaction_handlers_{};
  counter_map_type             transaction_counters_{};
  counter_map_type             query_counters_{};

  storage_type *state_ = nullptr;
};

}  // namespace ledger
}  // namespace fetch
