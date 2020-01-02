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

#include "chain/transaction.hpp"
#include "chain/transaction_layout_rpc_serializers.hpp"
#include "chain/transaction_rpc_serializers.hpp"
#include "ledger/storage_unit/transaction_storage_engine_interface.hpp"
#include "ledger/storage_unit/transaction_storage_protocol.hpp"
#include "logging/logging.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/registry.hpp"
#include "telemetry/utils/timer.hpp"

#include <sstream>

namespace fetch {
namespace ledger {
namespace {

using fetch::telemetry::FunctionTimer;
using Labels = telemetry::Measurement::Labels;

}  // namespace

/**
 * Build the Transaction Storage Protocol
 *
 * @param storage The reference to the storage engine
 * @param lane The lane / shard being targetted
 */
TransactionStorageProtocol::TransactionStorageProtocol(TransactionStorageEngineInterface &storage,
                                                       uint32_t                           lane)
  : lane_{lane}
  , storage_{storage}
  , add_total_{CreateCounter("add")}
  , has_total_{CreateCounter("has")}
  , get_total_{CreateCounter("get")}
  , get_count_total_{CreateCounter("get_count")}
  , get_recent_total_{CreateCounter("get_recent")}
  , add_durations_{CreateHistogram("add")}
  , has_durations_{CreateHistogram("has")}
  , get_durations_{CreateHistogram("get")}
  , get_count_durations_{CreateHistogram("get_count")}
  , get_recent_durations_{CreateHistogram("get_recent")}
{
  Expose(ADD, this, &TransactionStorageProtocol::Add);
  Expose(HAS, this, &TransactionStorageProtocol::Has);
  Expose(GET, this, &TransactionStorageProtocol::Get);
  Expose(GET_COUNT, this, &TransactionStorageProtocol::GetCount);
  Expose(GET_RECENT, this, &TransactionStorageProtocol::GetRecent);
}

/**
 * Create a total metric for a specified operation
 *
 * @param operation The operation
 * @return The generated counter
 */
telemetry::CounterPtr TransactionStorageProtocol::CreateCounter(char const *operation) const
{
  std::ostringstream name, description;
  name << "ledger_tx_store_" << operation << "_total";
  description << "The total number of '" << operation << "' operations";

  Labels const labels{{"lane", std::to_string(lane_)}};

  return telemetry::Registry::Instance().CreateCounter(name.str(), description.str(), labels);
}

/**
 * Create a duration histogram for a specified operation
 *
 * @param operation The operation
 * @return The generated histogram
 */
telemetry::HistogramPtr TransactionStorageProtocol::CreateHistogram(char const *operation) const
{
  std::ostringstream name, description;
  name << "ledger_tx_store_" << operation << "_duration";
  description << "The histogram of '" << operation << "' operation durations in seconds";

  Labels const labels{{"lane", std::to_string(lane_)}};

  return telemetry::Registry::Instance().CreateHistogram(
      {0.000001, 0.00001, 0.0001, 0.001, 0.01, 0.1, 1, 10., 100.}, name.str(), description.str(),
      labels);
}

/**
 * Add a new transaction to the storage engine
 *
 * @param tx The transaction to be stored
 * @param is_recent flag to signal if it is a recently seen transaction
 */
void TransactionStorageProtocol::Add(chain::Transaction const &tx)
{
  add_total_->increment();

  FunctionTimer timer{*add_durations_};
  storage_.Add(tx, false);
}

/**
 * Query if a transaction is present in the storage engine
 *
 * @param tx_digest The digest of the transaction being queried
 * @return true if the transaction is present, otherwise false
 */
bool TransactionStorageProtocol::Has(Digest const &tx_digest)
{
  has_total_->increment();

  FunctionTimer timer{*has_durations_};
  return storage_.Has(tx_digest);
}

/**
 * Retrieve a transaction from the storage engine
 *
 * @param tx_digest The digest of the transaction being queried
 * @param tx The output transaction to be populated
 * @return The transaction if successful, otherwise will throw an exception
 */
chain::Transaction TransactionStorageProtocol::Get(Digest const &tx_digest)
{
  get_total_->increment();

  FunctionTimer      timer{*get_durations_};
  chain::Transaction tx{};

  if (!storage_.Get(tx_digest, tx))
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Unable to lookup transaction 0x", tx_digest.ToHex());
    throw std::runtime_error{"Unable to lookup transaction"};
  }

  // once we have retrieved a transaction from the core it is important that we persist it to disk
  storage_.Confirm(tx_digest);

  return tx;
}

/**
 * Get the total number of stored transactions in this storage engine
 *
 * @return The total number of transactions
 */
uint64_t TransactionStorageProtocol::GetCount()
{
  get_count_total_->increment();

  FunctionTimer timer{*get_count_durations_};
  return storage_.GetCount();
}

/**
 * Get a set of transaction layouts corresponding to the most recent transactions received
 *
 * @param max_to_poll The maximum number of transactions be returned
 * @return The set of transaction layouts for the most recent transactions seen
 */
TransactionStorageProtocol::TxLayouts TransactionStorageProtocol::GetRecent(uint32_t max_to_poll)
{
  get_recent_total_->increment();

  FunctionTimer timer{*get_recent_durations_};
  return storage_.GetRecent(max_to_poll);
}

}  // namespace ledger
}  // namespace fetch
