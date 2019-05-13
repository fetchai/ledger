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

#include "ledger/transaction_verifier.hpp"
#include "core/logger.hpp"
#include "core/threading.hpp"
#include "metrics/metrics.hpp"
#include "network/generics/milli_timer.hpp"

#include <chrono>

static const std::chrono::milliseconds POP_TIMEOUT{300};
static const std::chrono::milliseconds WAITTIME_FOR_NEW_VERIFIED_TRANSACTIONS{1000};
static const std::chrono::milliseconds WAITTIME_FOR_NEW_VERIFIED_TRANSACTIONS_IF_FLUSH_NEEDED{1};

namespace fetch {
namespace ledger {

TransactionVerifier::~TransactionVerifier()
{
  // ensure that the verifier has been stopped
  Stop();
}

/**
 * Start the processor
 */
void TransactionVerifier::Start()
{
  // reserve the space required for all threads
  threads_.reserve(verifying_threads_ + 1);

  // create the verifier threads
  for (std::size_t i = 0, end = verifying_threads_; i < end; ++i)
  {
    threads_.emplace_back(std::make_unique<std::thread>([this, i]() {
      SetThreadName(name_ + "-V:", i);
      Verifier();
    }));
  }

  // create the dispatcher
  threads_.emplace_back(std::make_unique<std::thread>(&TransactionVerifier::Dispatcher, this));
}

/**
 * Stop the processor
 */
void TransactionVerifier::Stop()
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
void TransactionVerifier::Verifier()
{
  MutableTransaction mtx;

  bool success{false};

  while (active_)
  {
    try
    {
      // wait for a mutable transaction to be available
      if (unverified_queue_.Pop(mtx, POP_TIMEOUT))
      {
        // convert the transaction to a verified one and enqueue
        auto const tx = VerifiedTransaction::Create(mtx, &success);

        // check the status
        if (success)
        {
          verified_queue_.Push(tx);
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, name_ + " Unable to verify transaction: ",
                         byte_array::ToBase64(tx.digest()));
        }
      }
    }
    catch (std::exception const &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, name_ + " Exception caught: ", e.what());
    }
  }
}

/**
 * Internal: Dispatch thread process for verified transactions to be sent to the storage
 * engine and the mining interface.
 */
void TransactionVerifier::Dispatcher()
{
  SetThreadName(name_ + "-D");

  std::vector<VerifiedTransaction> txs;

  while (active_)
  {

    try
    {
      while (txs.size() < batch_size_ && active_)
      {
        std::chrono::milliseconds wait_time{WAITTIME_FOR_NEW_VERIFIED_TRANSACTIONS_IF_FLUSH_NEEDED};
        VerifiedTransaction       tx;
        if (txs.empty())
        {
          wait_time = WAITTIME_FOR_NEW_VERIFIED_TRANSACTIONS;
        }
        if (verified_queue_.Pop(tx, wait_time))
        {
          txs.emplace_back(std::move(tx));
        }
        else
        {
          break;
        }
      }

      switch (txs.size())
      {
      case 0:
        break;
      case 1:
        sink_.OnTransaction(txs.front());
        txs.clear();
        break;
      default:
        sink_.OnTransactions(txs);
        txs.clear();
        break;
      }
    }
    catch (std::exception const &e)
    {
      FETCH_LOG_WARN(LOGGING_NAME, name_ + " Exception caught: ", e.what());
    }
  }
}

}  // namespace ledger
}  // namespace fetch
