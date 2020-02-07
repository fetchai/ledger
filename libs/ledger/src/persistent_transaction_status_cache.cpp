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

#include "persistent_transaction_status_cache.hpp"

#include "core/serializers/main_serializer.hpp"
#include "logging/logging.hpp"

namespace fetch {

namespace serializers {

template <typename D>
struct MapSerializer<fetch::ledger::StakeUpdateEvent, D>
{
public:
  using Type       = fetch::ledger::StakeUpdateEvent;
  using DriverType = D;

  static uint8_t const BLOCK_INDEX = 1;
  static uint8_t const FROM        = 2;
  static uint8_t const AMOUNT      = 3;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &status)
  {
    auto map = map_constructor(3);
    map.Append(BLOCK_INDEX, status.block_index);
    map.Append(FROM, status.from);
    map.Append(AMOUNT, status.amount);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &status)
  {
    map.ExpectKeyGetValue(BLOCK_INDEX, status.block_index);
    map.ExpectKeyGetValue(FROM, status.from);
    map.ExpectKeyGetValue(AMOUNT, status.amount);
  }
};

template <typename D>
struct MapSerializer<fetch::ledger::ContractExecutionResult, D>
{
public:
  using Type       = fetch::ledger::ContractExecutionResult;
  using StatusEnum = fetch::ledger::ContractExecutionStatus;
  using DriverType = D;

  static uint8_t const STATUS        = 1;
  static uint8_t const CHARGE_USED   = 2;
  static uint8_t const CHARGE_RATE   = 3;
  static uint8_t const CHARGE_LIMIT  = 4;
  static uint8_t const FEE_CHARGED   = 5;
  static uint8_t const EXIT_CODE     = 6;
  static uint8_t const STAKE_UPDATES = 7;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &status)
  {
    auto map = map_constructor(7);
    map.Append(STATUS, static_cast<uint8_t>(status.status));
    map.Append(CHARGE_USED, status.charge);
    map.Append(CHARGE_RATE, status.charge_rate);
    map.Append(CHARGE_LIMIT, status.charge_limit);
    map.Append(FEE_CHARGED, status.fee);
    map.Append(EXIT_CODE, status.return_value);
    map.Append(STAKE_UPDATES, status.stake_updates);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &status)
  {
    DeserializeStatus(map, status.status);
    map.ExpectKeyGetValue(CHARGE_USED, status.charge);
    map.ExpectKeyGetValue(CHARGE_RATE, status.charge_rate);
    map.ExpectKeyGetValue(CHARGE_LIMIT, status.charge_limit);
    map.ExpectKeyGetValue(FEE_CHARGED, status.fee);
    map.ExpectKeyGetValue(EXIT_CODE, status.return_value);
    map.ExpectKeyGetValue(STAKE_UPDATES, status.stake_updates);
  }

  template <typename MapDeserializer>
  static void DeserializeStatus(MapDeserializer &map, StatusEnum &status)
  {
    // read the raw value
    uint8_t raw_enum_value{0};
    map.ExpectKeyGetValue(STATUS, raw_enum_value);

    // convert the raw value
    auto converted_value = static_cast<StatusEnum>(raw_enum_value);

    switch (converted_value)
    {
    case StatusEnum::SUCCESS:
    case StatusEnum::TX_LOOKUP_FAILURE:
    case StatusEnum::TX_NOT_VALID_FOR_BLOCK:
    case StatusEnum::TX_PERMISSION_DENIED:
    case StatusEnum::TX_NOT_ENOUGH_CHARGE:
    case StatusEnum::TX_CHARGE_LIMIT_TOO_HIGH:
    case StatusEnum::INSUFFICIENT_AVAILABLE_FUNDS:
    case StatusEnum::CONTRACT_NAME_PARSE_FAILURE:
    case StatusEnum::CONTRACT_LOOKUP_FAILURE:
    case StatusEnum::ACTION_LOOKUP_FAILURE:
    case StatusEnum::CONTRACT_EXECUTION_FAILURE:
    case StatusEnum::TRANSFER_FAILURE:
    case StatusEnum::INSUFFICIENT_CHARGE:
    case StatusEnum::NOT_RUN:
    case StatusEnum::INTERNAL_ERROR:
    case StatusEnum::INEXPLICABLE_FAILURE:
      status = converted_value;
      return;
    }

    throw std::runtime_error("Unable to convert status enum");
  }
};

template <typename D>
struct MapSerializer<fetch::ledger::TransactionStatusInterface::TxStatus, D>
{
public:
  using Type       = fetch::ledger::TransactionStatusInterface::TxStatus;
  using StatusEnum = fetch::ledger::TransactionStatus;
  using DriverType = D;

  static uint8_t const STATUS           = 1;
  static uint8_t const EXECUTION_RESULT = 2;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &status)
  {
    auto map = map_constructor(2);
    map.Append(STATUS, static_cast<uint8_t>(status.status));
    map.Append(EXECUTION_RESULT, status.contract_exec_result);
  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &status)
  {
    DeserializeStatus(map, status.status);
    map.ExpectKeyGetValue(EXECUTION_RESULT, status.contract_exec_result);
  }

  template <typename MapDeserializer>
  static void DeserializeStatus(MapDeserializer &map, StatusEnum &status)
  {
    // read the raw value
    uint8_t raw_enum_value{0};
    map.ExpectKeyGetValue(EXECUTION_RESULT, raw_enum_value);

    // convert the raw value
    auto converted_value = static_cast<StatusEnum>(raw_enum_value);

    switch (converted_value)
    {
    case StatusEnum::UNKNOWN:
    case StatusEnum::PENDING:
    case StatusEnum::MINED:
    case StatusEnum::EXECUTED:
    case StatusEnum::SUBMITTED:
      status = converted_value;
      return;
    }

    throw std::runtime_error("Unable to convert status enum");
  }
};

}  // namespace serializers

namespace ledger {
namespace {

constexpr char const *LOGGING_NAME = "PersistentTxCache";

using TxStatus = PersistentTransactionStatusCache::TxStatus;

storage::ResourceID CreateRID(Digest digest)
{
  return storage::ResourceID{digest};
}

}  // namespace

/**
 * Create an instance of the persistent transaction status cache
 *
 * @param mode The operating mode for the database
 */
PersistentTransactionStatusCache::PersistentTransactionStatusCache(Mode mode)
{
  // open the database to the correct mode
  switch (mode)
  {
  case Mode::NEW_DATABASE:
    store_.New("tx-status.db", "tx-status.index.db", true);
    break;

  case Mode::LOAD_EXISTING:
    store_.Load("tx-status.db", "tx-status.index.db", true);
    break;
  }
}

/**
 * Query the status of a specified transaction
 *
 * @param digest The digest of the transaction
 * @return The status object associated for this transaction
 */
TxStatus PersistentTransactionStatusCache::Query(Digest digest) const
{
  FETCH_LOCK(lock_);
  return LookupStatus(digest);
}

/**
 * Update the status of a transaction with the specified status enum
 *
 * @param digest The transaction to be updated
 * @param status The status value that should be set
 */
void PersistentTransactionStatusCache::Update(Digest digest, TransactionStatus status)
{
  // this method should not be used to update the execution status
  if (status == TransactionStatus::EXECUTED)
  {
    FETCH_LOG_WARN("TransactionStatusCache",
                   "Using inappropriate method to update contract "
                   "execution result. (tx digest: 0x",
                   digest.ToHex(), ")");

    throw std::runtime_error(
        "TransactionStatusCache::Update(...): Using inappropriate method to update"
        "contract execution result");
  }

  FETCH_LOCK(lock_);

  // lookup the existing (if any status) for this transaction
  auto retrieved_status = LookupStatus(digest);

  // update the status
  retrieved_status.status = status;

  // flush the status changes down to disk
  UpdateStatus(digest, retrieved_status);
}

/**
 * Update the contract execution result for the specified transaction
 *
 * @param digest The transaction to be updated
 * @param exec_result The contract execution result
 */
void PersistentTransactionStatusCache::Update(Digest digest, ContractExecutionResult exec_result)
{
  FETCH_LOCK(lock_);

  // lookup the existing (if any status) for this transaction
  auto retrieved_status = LookupStatus(digest);

  // update the status
  retrieved_status.status               = TransactionStatus::EXECUTED;
  retrieved_status.contract_exec_result = exec_result;

  // flush the status changes down to disk
  UpdateStatus(digest, retrieved_status);
}

/**
 * Attempt to lookup a previously stored transaction status from the disk
 *
 * @param digest The digest to lookup
 * @return The returned status, or default if it fails
 */
TxStatus PersistentTransactionStatusCache::LookupStatus(Digest digest) const
{
  TxStatus status{};

  try
  {
    store_.Get(CreateRID(digest), status);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Error looking up status for tx: 0x", digest.ToHex(), " : ",
                   ex.what());
  }

  return status;
}

/**
 * Update the stored transaction status on disk
 *
 * @param digest The digest of the transaction
 * @param status The status information
 */
void PersistentTransactionStatusCache::UpdateStatus(Digest digest, TxStatus status)
{
  try
  {
    store_.Set(CreateRID(digest), status);
  }
  catch (std::exception const &ex)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Error saving status for tx: 0x", digest.ToHex(), " : ",
                   ex.what());
  }
}

}  // namespace ledger
}  // namespace fetch
