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

#include "ledger/transaction_processor.hpp"
#include "core/threading.hpp"
#include "ledger/block_packer_interface.hpp"
#include "ledger/storage_unit/storage_unit_interface.hpp"
#include "metrics/metrics.hpp"

namespace fetch {
namespace ledger {

/**
 * Transaction Processor constructor
 *
 * @param storage The reference to the storage unit
 * @param miner The reference to the system miner
 */
TransactionProcessor::TransactionProcessor(StorageUnitInterface &storage,
                                           BlockPackerInterface &packer, std::size_t num_threads)
  : storage_{storage}
  , packer_{packer}
  , verifier_{*this, num_threads, "TxV-P"}
  , running_{false}
{}

TransactionProcessor::~TransactionProcessor()
{
  Stop();
}

void TransactionProcessor::OnTransaction(UnverifiedTransaction const &tx)
{
  // submit the transaction to the verifier - it will call back this::OnTransaction(verified) or
  // this::OnTransactions(verified)
  verifier_.AddTransaction(tx.AsMutable());
}

void TransactionProcessor::OnTransaction(VerifiedTransaction const &tx)
{
  FETCH_METRIC_TX_SUBMITTED(tx.digest());

  FETCH_LOG_DEBUG(LOGGING_NAME, "Verified Input Transaction: ", byte_array::ToBase64(tx.digest()), " (", tx.contract_name(), ')');

  // dispatch the transaction to the storage engine
  try
  {
    storage_.AddTransaction(tx);
  }
  catch (std::runtime_error &e)
  {
    // TODO(unknown): We need to think about how we handle failures of that class.
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to add transaction to storage: ", e.what());
    return;
  }

  FETCH_METRIC_TX_STORED(tx.digest());

  // dispatch the summary to the miner
  packer_.EnqueueTransaction(tx.summary());

  FETCH_METRIC_TX_QUEUED(tx.digest());
}

void TransactionProcessor::OnTransactions(TransactionList const &txs)
{
#ifdef FETCH_ENABLE_METRICS
  auto const submitted = metrics::Metrics::Clock::now();
#endif  // FETCH_ENABLE_METRICS

  // dispatch all the transactions to the storage engine
  try
  {
    storage_.AddTransactions(txs);
  }
  catch (std::runtime_error &e)
  {
    // TODO(unknown): We need to think about how we handle failures of that class.
    FETCH_LOG_WARN(LOGGING_NAME, "Failed to add transaction to storage: ", e.what());
    return;
  }

#ifdef FETCH_ENABLE_METRICS
  auto const stored = metrics::Metrics::Clock::now();
#endif  // FETCH_ENABLE_METRICS

  // enqueue all of the transactions
  for (auto const &tx : txs)
  {
    packer_.EnqueueTransaction(tx.summary());
  }

#ifdef FETCH_ENABLE_METRICS
  auto const queued = metrics::Metrics::Clock::now();

  for (auto const &tx : txs)
  {
    FETCH_METRIC_TX_SUBMITTED_EX(tx.digest(), submitted);
    FETCH_METRIC_TX_STORED_EX(tx.digest(), stored);
    FETCH_METRIC_TX_QUEUED_EX(tx.digest(), queued);
  }
#endif  // FETCH_ENABLE_METRICS
}

void TransactionProcessor::ThreadEntryPoint()
{
  SetThreadName("TxProc");

  std::vector<TransactionSummary> new_txs;
  while (running_)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    new_txs.clear();
    new_txs = storage_.PollRecentTx(10000);

    if (!new_txs.empty())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Pulled ", new_txs.size(), " transactions from shards");
    }

    for (auto const &summary : new_txs)
    {
      // Note: metric for TX stored will not fire this way
      // dispatch the summary to the miner
      assert(summary.IsWellFormed());
      packer_.EnqueueTransaction(summary);

      FETCH_METRIC_TX_QUEUED(summary.transaction_hash);
    }
  }
}

}  // namespace ledger
}  // namespace fetch
