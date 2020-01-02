#pragma once
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

#include "chain/address.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/identity.hpp"
#include "ledger/chaincode/contract_context.hpp"
#include "ledger/fees/chargeable.hpp"
#include "ledger/state_adapter.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

namespace fetch {

namespace variant {
class Variant;
}

namespace chain {

class Transaction;

}  // namespace chain

namespace ledger {

/**
 * Contract - Base class for all smart contract and chain code instances
 */
class Contract : public Chargeable
{
public:
  enum class Status
  {
    OK = 0,
    FAILED,
    NOT_FOUND,
  };

  struct Result
  {
    Status   status{Status::NOT_FOUND};
    int64_t  return_value{0};
    uint64_t block_index{0};
  };

  using BlockIndex     = chain::TransactionLayout::BlockIndex;
  using Identity       = crypto::Identity;
  using ConstByteArray = byte_array::ConstByteArray;
  using ContractName   = ConstByteArray;
  using Query          = variant::Variant;
  using InitialiseHandler =
      std::function<Result(chain::Address const &, chain::Transaction const &)>;
  using TransactionHandler    = std::function<Result(chain::Transaction const &)>;
  using TransactionHandlerMap = std::unordered_map<ContractName, TransactionHandler>;
  using QueryHandler          = std::function<Status(Query const &, Query &)>;
  using QueryHandlerMap       = std::unordered_map<ContractName, QueryHandler>;
  using Counter               = std::atomic<std::size_t>;
  using CounterMap            = std::unordered_map<ContractName, Counter>;
  using StorageInterface      = ledger::StorageInterface;

  // Construction / Destruction
  Contract()                 = default;
  Contract(Contract const &) = delete;
  Contract(Contract &&)      = delete;
  ~Contract() override       = default;

  /// @name Contract Lifecycle Handlers
  /// @{
  Result DispatchInitialise(chain::Address const &owner, chain::Transaction const &tx);
  Status DispatchQuery(ContractName const &name, Query const &query, Query &response);
  Result DispatchTransaction(chain::Transaction const &tx);
  /// @}

  ContractContext const &context() const;

  /// @name Dispatch Maps Accessors
  /// @{
  QueryHandlerMap const &      query_handlers() const;
  TransactionHandlerMap const &transaction_handlers() const;
  /// @}

  uint64_t CalculateFee() const override
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
  void OnInitialise(C *instance,
                    Result (C::*func)(chain::Address const &, chain::Transaction const &));
  /// @}

  /// @name Transaction Handlers
  /// @{
  void OnTransaction(std::string const &name, TransactionHandler &&handler);
  template <typename C>
  void OnTransaction(std::string const &name, C *instance,
                     Result (C::*func)(chain::Transaction const &));
  /// @}

  /// @name Query Handler Registration
  /// @{
  void OnQuery(std::string const &name, QueryHandler &&handler);
  template <typename C>
  void OnQuery(std::string const &name, C *instance, Status (C::*func)(Query const &, Query &));
  /// @}

  /// @name Chain Code State Utils
  /// @{
  bool ParseAsJson(chain::Transaction const &tx, variant::Variant &output);

  template <typename T>
  bool GetStateRecord(T &record, ConstByteArray const &key);
  template <typename T>
  bool GetStateRecord(T &record, chain::Address const &address);
  template <typename T>
  StateAdapter::Status SetStateRecord(T const &record, ConstByteArray const &key);
  template <typename T>
  StateAdapter::Status SetStateRecord(T const &record, chain::Address const &address);

  ledger::StateAdapter &state();
  /// @}

private:
  void Attach(ContractContext context);
  void Detach();

  std::unique_ptr<ContractContext> context_{};

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

  friend class ContractContextAttacher;
};

/**
 * Register a class member function as an init handler
 *
 * @tparam C The class type
 * @param instance The pointer to the class instance
 * @param func The member function pointer
 */
template <typename C>
void Contract::OnInitialise(C *instance,
                            Result (C::*func)(chain::Address const &, chain::Transaction const &))
{
  OnInitialise([instance, func](chain::Address const &owner, chain::Transaction const &tx) {
    return (instance->*func)(owner, tx);
  });
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
                             Result (C::*func)(chain::Transaction const &))
{
  // create the function handler and pass it to the normal function
  OnTransaction(name,
                [instance, func](chain::Transaction const &tx) { return (instance->*func)(tx); });
}

/**
 * Register class member query handler
 *
 * @tparam C The class type
 * @param name The query name
 * @param instance The pointer to the class instance
 * @param func The member function pointer
 */
template <typename C>
void Contract::OnQuery(std::string const &name, C *instance,
                       Status (C::*func)(Query const &, Query &))
{
  OnQuery(name, [instance, func](Query const &query, Query &response) {
    return (instance->*func)(query, response);
  });
}

template <typename T>
bool Contract::GetStateRecord(T &record, chain::Address const &address)
{
  return GetStateRecord(record, address.display());
}

/**
 * Look up the state record stored with the specified key
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
    serializers::MsgPackSerializer adapter{buffer};
    adapter >> record;

    success = true;
    break;
  }
  case vm::IoObserverInterface::Status::ERROR:
  case vm::IoObserverInterface::Status::PERMISSION_DENIED:
  case vm::IoObserverInterface::Status::BUFFER_TOO_SMALL:
    break;
  }

  return success;
}

template <typename T>
StateAdapter::Status Contract::SetStateRecord(T const &record, chain::Address const &address)
{
  return SetStateRecord(record, address.display());
}

/**
 * Store a state record at a specified key
 *
 * @tparam T The type of the state record
 * @param record The reference to the record
 * @param key The key for the state record
 */
template <typename T>
StateAdapter::Status Contract::SetStateRecord(T const &record, ConstByteArray const &key)
{
  // serialize the record to the buffer
  serializers::MsgPackSerializer buffer;
  buffer << record;

  // look up reference to the underlying buffer
  auto const &data = buffer.data();

  // store the buffer
  return state().Write(std::string{key}, data.pointer(), data.size());
}

}  // namespace ledger
}  // namespace fetch
