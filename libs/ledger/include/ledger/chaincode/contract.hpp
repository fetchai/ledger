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
#include "crypto/identity.hpp"
#include "ledger/identifier.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "variant/variant.hpp"

#include "ledger/state_adapter.hpp"

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

class Transaction;
class Address;

/**
 * Contract - Base class for all smart contract and chain code instances
 */
class Contract
{
public:
  enum class Status
  {
    OK = 0,
    FAILED,
    NOT_FOUND,
  };

  using Identity              = crypto::Identity;
  using ConstByteArray        = byte_array::ConstByteArray;
  using ContractName          = ConstByteArray;
  using Query                 = variant::Variant;
  using InitialiseHandler     = std::function<Status(Address const &)>;
  using TransactionHandler    = std::function<Status(Transaction const &)>;
  using TransactionHandlerMap = std::unordered_map<ContractName, TransactionHandler>;
  using QueryHandler          = std::function<Status(Query const &, Query &)>;
  using QueryHandlerMap       = std::unordered_map<ContractName, QueryHandler>;
  using Counter               = std::atomic<std::size_t>;
  using CounterMap            = std::unordered_map<ContractName, Counter>;
  using StorageInterface      = ledger::StorageInterface;

  static constexpr char const *LOGGING_NAME = "Contract";

  // Construction / Destruction
  Contract()                 = default;
  Contract(Contract const &) = delete;
  Contract(Contract &&)      = delete;
  virtual ~Contract()        = default;

  /// @name Contract Lifecycle Handlers
  /// @{
  void Attach(ledger::StateAdapter &state);
  void Detach();

  Status DispatchInitialise(Address const &owner);
  Status DispatchQuery(ContractName const &name, Query const &query, Query &response);
  Status DispatchTransaction(ConstByteArray const &name, Transaction const &tx);
  /// @}

  /// @name Dispatch Maps Accessors
  /// @{
  QueryHandlerMap const &      query_handlers() const;
  TransactionHandlerMap const &transaction_handlers() const;
  /// @}

  virtual uint64_t CalculateFee() const
  {
    return 0;
  }

  // Operators
  Contract &operator=(Contract const &) = delete;
  Contract &operator=(Contract &&) = delete;

protected:
  /// @name Initialise Handlers
  /// @{
  void OnInitialise(InitialiseHandler &&handler);
  template <typename C>
  void OnInitialise(C *instance, Status (C::*func)(Address const &));
  /// @}

  /// @name Transaction Handlers
  /// @{
  void OnTransaction(std::string const &name, TransactionHandler &&handler);
  template <typename C>
  void OnTransaction(std::string const &name, C *instance, Status (C::*func)(Transaction const &));
  /// @}

  /// @name Query Handler Registration
  /// @{
  void OnQuery(std::string const &name, QueryHandler &&handler);
  template <typename C>
  void OnQuery(std::string const &name, C *instance, Status (C::*func)(Query const &, Query &));
  /// @}

  /// @name Chain Code State Utils
  /// @{
  bool ParseAsJson(Transaction const &tx, variant::Variant &output);

  template <typename T>
  bool GetStateRecord(T &record, ConstByteArray const &key);
  template <typename T>
  void SetStateRecord(T const &record, ConstByteArray const &key);

  ledger::StateAdapter &state();
  /// @}

private:
  static constexpr std::size_t DEFAULT_BUFFER_SIZE = 512;

  /// @name Dispatch Maps - built on construction
  /// @{
  InitialiseHandler     init_handler_{};
  QueryHandlerMap       query_handlers_{};
  TransactionHandlerMap transaction_handlers_{};
  /// @}

  /// @name Statistics
  CounterMap transaction_counters_{};
  CounterMap query_counters_{};
  /// @}

  /// @name State
  /// @{
  ledger::StateAdapter *state_ = nullptr;
  /// @}
};

/**
 * Attach the state interface to the contract instance
 *
 * @param state The reference
 */
inline void Contract::Attach(ledger::StateAdapter &state)
{
  state_ = &state;
}

/**
 * Detach the state interface from the contract instance
 */
inline void Contract::Detach()
{
  state_ = nullptr;
}

/**
 * Query Handler Map Accessor
 *
 * @return The query handler map
 */
inline Contract::QueryHandlerMap const &Contract::query_handlers() const
{
  return query_handlers_;
}

/**
 * Transaction Handler Map Accessor
 *
 * @return The transaction handler map
 */
inline Contract::TransactionHandlerMap const &Contract::transaction_handlers() const
{
  return transaction_handlers_;
}

/**
 * Register a class member function as an init handler
 *
 * @tparam C The class type
 * @param instance The pointer to the class instance
 * @param func The member function pointer
 */
template <typename C>
void Contract::OnInitialise(C *instance, Status (C::*func)(Address const &))
{
  OnInitialise([instance, func](Address const &owner) { return (instance->*func)(owner); });
}

/**
 * Register class member function transaction handler
 *
 * @tparam C The class type
 * @param name The action name
 * @param instance The pointer to the class instance
 * @param func The member function pointer
 */
template <typename C>
void Contract::OnTransaction(std::string const &name, C *instance,
                             Status (C::*func)(Transaction const &))
{
  // create the function handler and pass it to the normal function
  OnTransaction(name, [instance, func](Transaction const &tx) { return (instance->*func)(tx); });
}

/**
 * Register class member query handler
 *
 * @tparam C The class type
 * @param name THe query name
 * @param instance The pointer to the class instance
 * @param func THe member function pointer
 */
template <typename C>
void Contract::OnQuery(std::string const &name, C *instance,
                       Status (C::*func)(Query const &, Query &))
{
  OnQuery(name, [instance, func](Query const &query, Query &response) {
    return (instance->*func)(query, response);
  });
}

/**
 * Lookup the state record stored with the specified key
 *
 * @tparam T The type of the state record
 * @param record The reference to the record to be populated
 * @param key The key of the index
 * @return true if successful, otherwise false
 */
template <typename T>
bool Contract::GetStateRecord(T &record, ConstByteArray const &key)
{
  using fetch::byte_array::ByteArray;

  bool success{false};

  ByteArray buffer;
  buffer.Resize(std::size_t{DEFAULT_BUFFER_SIZE});  // initial guess, can be tuned over time

  uint64_t buffer_length = buffer.size();
  auto     status        = state().Read(std::string{key}, buffer.pointer(), buffer_length);

  // in rare cases the initial buffer might be too small in this case we need to reallocate and then
  // re-query
  if (vm::IoObserverInterface::Status::BUFFER_TOO_SMALL == status)
  {
    // reallocate the buffer
    buffer.Resize(buffer_length);

    // retry the read
    status = state().Read(std::string{key}, buffer.pointer(), buffer_length);
  }

  switch (status)
  {
  case vm::IoObserverInterface::Status::OK:
  {
    // adapt the buffer for deserialization
    serializers::ByteArrayBuffer adapter{buffer};
    adapter >> record;

    success = true;
    break;
  }
  case vm::IoObserverInterface::Status::ERROR:
    break;
  case vm::IoObserverInterface::Status::PERMISSION_DENIED:
    break;
  case vm::IoObserverInterface::Status::BUFFER_TOO_SMALL:
    break;
  }

  return success;
}

/**
 * Store a state record at a specified key
 *
 * @tparam T The type of the state record
 * @param record The reference to the record
 * @param key The key for the state record
 */
template <typename T>
void Contract::SetStateRecord(T const &record, ConstByteArray const &key)
{
  // serialize the record to the buffer
  serializers::ByteArrayBuffer buffer;
  buffer << record;

  // lookup reference to the underlying buffer
  auto const &data = buffer.data();

  // store the buffer
  state().Write(std::string{key}, data.pointer(), data.size());
}

}  // namespace ledger
}  // namespace fetch
