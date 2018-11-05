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

#include "ledger/transaction_processor.hpp"

namespace fetch {
namespace ledger {

/**
 * Transaction Processor constructor
 *
 * @param storage The reference to the storage unit
 * @param miner The reference to the system miner
 */
TransactionProcessor::TransactionProcessor(StorageUnitInterface & storage,
                                           miner::MinerInterface &miner)
  : storage_{storage}
  , miner_{miner}
{}

/**
 * Start the processor
 */
void TransactionProcessor::Start()
{
  std::size_t const num_verifier_threads = 2 * std::thread::hardware_concurrency();

  // reserve the space required for all threads
  threads_.reserve(num_verifier_threads + 1);

  // create the verifier threads
  for (std::size_t i = 0, end = 2 * std::thread::hardware_concurrency(); i < end; ++i)
  {
    threads_.emplace_back(std::make_unique<std::thread>(&TransactionProcessor::Verifier, this));
  }

  // create the dispatcher
  threads_.emplace_back(std::make_unique<std::thread>(&TransactionProcessor::Dispatcher, this));
}

/**
 * Stop the processor
 */
void TransactionProcessor::Stop()
{
  // signal the worker threads to stop
  active_ = false;

  // wait for the threads to complete
  for (auto &thread : threads_)
  {
    thread->join();
    thread.reset();
  }
  threads_.clear();
}

/**
 * Internal: Thread process for the verification of
 */
void TransactionProcessor::Verifier()
{
  MutableTransaction mtx;

  while (active_)
  {
    // wait for a mutable transaction to be available
    if (unverified_queue_.Pop(mtx, std::chrono::milliseconds{300}))
    {
      // convert the transaction to a verified one and enqueue
      verified_queue_.Push(chain::VerifiedTransaction::Create(mtx));
    }
  }
}

/**
 * Internal: Dispatch thread process for
 */
void TransactionProcessor::Dispatcher()
{
  static constexpr std::size_t MAXIMUM_BATCH_SIZE = 1000;

  std::vector<chain::VerifiedTransaction> txs;

  while (active_)
  {
    chain::VerifiedTransaction tx;

    bool dispatch  = false;
    bool populated = false;

    // the dispatcher works in two modes
    if (txs.empty())
    {
      // wait for a new element to be available
      populated = verified_queue_.Pop(tx, std::chrono::milliseconds{300});
    }
    else if (txs.size() >= MAXIMUM_BATCH_SIZE)
    {
      // signal the dispatch
      dispatch = true;
    }
    else
    {
      // since we know that there is at least one tx in the our local queue
      // attempt to create batches of the transactions up.
      populated = verified_queue_.Pop(tx, std::chrono::microseconds{1});

      // if we do not collect any more transactions then dispatch the batch
      dispatch = !populated;
    }

    // update our transaction list if needed
    if (populated)
    {
      txs.emplace_back(std::move(tx));
    }

    // if required dispatch all the transactions
    if (dispatch)
    {
      // add the transactions to the storage engine
      if (txs.size() == 1)
      {
        storage_.AddTransaction(txs[0]);

        FETCH_METRIC_TX_STORED(txs[0].digest());
      }
      else
      {
        storage_.AddTransactions(txs);

#ifdef FETCH_ENABLE_METRICS
        auto const stored = metrics::Metrics::Clock::now();

        for (auto const &tx : txs)
        {
          FETCH_METRIC_TX_STORED_EX(tx.digest(), stored);
        }
#endif  // FETCH_ENABLE_METRICS
      }

      // enqueue all the transactions
      for (auto const &tx : txs)
      {
        miner_.EnqueueTransaction(tx.summary());
      }

#ifdef FETCH_ENABLE_METRICS
      auto const queued = metrics::Metrics::Clock::now();

      for (auto const &tx : txs)
      {
        FETCH_METRIC_TX_QUEUED_EX(tx.digest(), queued);
      }
#endif  // FETCH_ENABLE_METRICS

      // clear the transaction list
      txs.clear();
    }
  }
}

}  // namespace ledger
}  // namespace fetch