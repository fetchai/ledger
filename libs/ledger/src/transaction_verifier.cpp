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

#include "core/logger.hpp"
#include "core/string/to_lower.hpp"
#include "core/threading.hpp"
#include "ledger/chain/transaction.hpp"
#include "ledger/storage_unit/transaction_sinks.hpp"
#include "ledger/transaction_verifier.hpp"
#include "metrics/metrics.hpp"
#include "network/generics/milli_timer.hpp"
#include "telemetry/counter.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/registry.hpp"

#include <algorithm>
#include <chrono>
#include <string>

static const std::chrono::milliseconds POP_TIMEOUT{300};

namespace fetch {
namespace ledger {

namespace {

using telemetry::Registry;

std::string CreateMetricName(std::string const &prefix, std::string const &name)
{
  // build up the basic name
  std::string metric_name = "ledger_" + prefix + '_' + name;

  // ensure there are no invalid characterss
  std::transform(metric_name.begin(), metric_name.end(), metric_name.begin(), [](char c) -> char {
    if (c == '-')
    {
      return '_';
    }
    else
    {
      return static_cast<char>(std::tolower(c));
    }
  });

  return metric_name;
}

telemetry::GaugePtr<uint64_t> CreateGauge(std::string const &prefix, std::string const &name,
                                          std::string const &description)
{
  std::string metric_name = CreateMetricName(prefix, name);
  return Registry::Instance().CreateGauge<uint64_t>(std::move(metric_name), description);
}

telemetry::CounterPtr CreateCounter(std::string const &prefix, std::string const &name,
                                    std::string const &description)
{
  std::string metric_name = CreateMetricName(prefix, name);
  return Registry::Instance().CreateCounter(std::move(metric_name), description);
}

}  // namespace

/**
 * Construct a transaction verifier queue
 *
 * @param sink The destination for verified transactions
 * @param verifying_threads The number of verifying threads to be used
 * @param name The name of the verifier
 */
TransactionVerifier::TransactionVerifier(TransactionSink &sink, std::size_t verifying_threads,
                                         std::string const &name)
  : verifying_threads_(verifying_threads)
  , name_(name)
  , sink_(sink)
  , unverified_queue_length_(
        CreateGauge(name, "unverified_queue_size", "The current size of the unverified queue"))
  , unverified_queue_max_length_(
        CreateGauge(name, "unverified_queue_max_size", "The max size of the unverified queue"))
  , verified_queue_length_(
        CreateGauge(name, "verified_queue_size", "The current size of the verified queue"))
  , verified_queue_max_length_(
        CreateGauge(name, "verified_queue_max_size", "The max size of the verified queue"))
  , unverified_tx_total_(CreateCounter(name, "unverified_transactions_total",
                                       "The total number of unverified transactions seen"))
  , verified_tx_total_(CreateCounter(name, "verified_transactions_total",
                                     "The total number of verified transactions seen"))
  , discarded_tx_total_(CreateCounter(name, "discarded_transactions_total",
                                      "The total number of verified transactions seen"))
  , dispatched_tx_total_(CreateCounter(name, "dispatched_transactions_total",
                                       "The total number of verified that have been dispatched"))
  , num_threads_(CreateGauge(name, "threads", "The current number of processing threads in use"))
{
  // since these lengths are fixed
  unverified_queue_max_length_->increment(std::size_t{QUEUE_SIZE});
  verified_queue_max_length_->increment(std::size_t{QUEUE_SIZE});
}

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
  num_threads_->increment(threads_.size());
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
 * Add a transaction into the processing queue
 *
 * @param tx The transaction to be added
 */
void TransactionVerifier::AddTransaction(TransactionPtr const &tx)
{
  unverified_queue_.Push(tx);
  unverified_queue_length_->increment();
  unverified_tx_total_->increment();
}

/**
 * Add a transaction into the processing queue
 *
 * @param tx The transaction to be added
 */
void TransactionVerifier::AddTransaction(TransactionPtr &&tx)
{
  unverified_queue_.Push(std::move(tx));
  unverified_queue_length_->increment();
  unverified_tx_total_->increment();
}

/**
 * Internal: Thread process for the verification of
 */
void TransactionVerifier::Verifier()
{
  TransactionPtr tx;

  while (active_)
  {
    try
    {
      // wait for a mutable transaction to be available
      if (unverified_queue_.Pop(tx, POP_TIMEOUT))
      {
        unverified_queue_length_->decrement();

        FETCH_LOG_DEBUG(LOGGING_NAME, "Verifying TX: 0x", tx->digest().ToHex());

        // check the status
        if (tx->Verify())
        {
          FETCH_LOG_DEBUG(LOGGING_NAME, "TX Verify Complete: 0x", tx->digest().ToHex());

          verified_queue_.Push(std::move(tx));
          verified_queue_length_->increment();
          verified_tx_total_->increment();
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, name_ + " Unable to verify transaction: 0x",
                         tx->digest().ToHex());

          discarded_tx_total_->increment();
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

  while (active_)
  {
    try
    {
      TransactionPtr tx;
      if (verified_queue_.Pop(tx, POP_TIMEOUT))
      {
        FETCH_LOG_DEBUG(LOGGING_NAME, "TX Dispatch: 0x", tx->digest().ToHex());

        sink_.OnTransaction(tx);

        verified_queue_length_->decrement();
        dispatched_tx_total_->increment();
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
