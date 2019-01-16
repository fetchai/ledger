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
#include "metrics/metrics.hpp"

namespace fetch {
namespace ledger {

/**
 * Transaction Processor constructor
 *
 * @param storage The reference to the storage unit
 * @param miner The reference to the system miner
 */
TransactionProcessor::TransactionProcessor(StorageUnitInterface & storage,
                                           miner::MinerInterface &miner, std::size_t num_threads)
  : storage_{storage}
  , miner_{miner}
  , verifier_{*this, num_threads}
{}

TransactionProcessor::~TransactionProcessor()
{
  running_ = false;
}

void TransactionProcessor::OnTransaction(chain::UnverifiedTransaction const &tx)
{
  // submit the transaction to the verifier - it will call back this::OnTransaction(verified) or this::OnTransactions(verified)
  verifier_.AddTransaction(tx.AsMutable());
}

void TransactionProcessor::OnTransaction(chain::VerifiedTransaction const &tx)
{
  FETCH_METRIC_TX_SUBMITTED(tx.digest());

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
  miner_.EnqueueTransaction(tx.summary());

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
    miner_.EnqueueTransaction(tx.summary());
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
  std::vector<chain::TransactionSummary> new_txs;
  while (running_)
  {
    new_txs.clear();
    new_txs = storage_.PollRecentTx(10000);
  }
}


}  // namespace ledger
}  // namespace fetch
