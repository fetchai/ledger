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

#include "ledger/metrics/metric_handler.hpp"

#include <string>

namespace fetch {
namespace ledger {

/**
 * Singleton object for convenient global access to generating metric data
 */
class Metrics
{
public:
  using Clock          = MetricHandler::Clock;
  using Timestamp      = Clock::time_point;
  using ConstByteArray = byte_array::ConstByteArray;
  using Instrument     = MetricHandler::Instrument;
  using Event          = MetricHandler::Event;

  // Singleton instance
  static Metrics &Instance();

  // Construction / Destruction
  Metrics(Metrics const &) = delete;
  Metrics(Metrics &&)      = delete;
  ~Metrics()
  {
    RemoveMetricHandler();
  }

  // Configuration
  void ConfigureFileHandler(std::string filename);

  void RecordMetric(ConstByteArray const &identifier, Instrument instrument, Event event,
                    Timestamp const &timestamp = Clock::now())
  {
    auto handler = handler_.load();
    if (handler)
    {
      handler->RecordMetric(identifier, instrument, event, timestamp);
    }
  }

  void RecordTransactionMetric(ConstByteArray const &hash, Event event,
                               Timestamp const &timestamp = Clock::now())
  {
    RecordMetric(hash, Instrument::TRANSACTION, event, timestamp);
  }

  // Operators
  Metrics &operator=(Metrics const &) = delete;
  Metrics &operator=(Metrics &&) = delete;

private:
  // Hidden construction
  Metrics() = default;
  void RemoveMetricHandler();

  std::unique_ptr<MetricHandler> handler_object_;
  std::atomic<MetricHandler *>   handler_{nullptr};
};

}  // namespace ledger
}  // namespace fetch

#ifdef FETCH_ENABLE_METRICS
#define FETCH_METRIC_TX_SUBMITTED(hash)                       \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::SUBMITTED)
#define FETCH_METRIC_TX_STORED(hash)                          \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::STORED)
#define FETCH_METRIC_TX_QUEUED(hash)                          \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::QUEUED)
#define FETCH_METRIC_TX_PACKED(hash)                          \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::PACKED)
#define FETCH_METRIC_TX_EXEC_STARTED(hash)                    \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::EXECUTION_STARTED)
#define FETCH_METRIC_TX_EXEC_COMPLETE(hash)                   \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::EXECUTION_COMPLETE)

#define FETCH_METRIC_TX_SUBMITTED_EX(hash, timepoint)         \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::SUBMITTED, timepoint)
#define FETCH_METRIC_TX_QUEUED_EX(hash, timepoint)            \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::QUEUED, timepoint)
#define FETCH_METRIC_TX_PACKED_EX(hash, timepoint)            \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::PACKED, timepoint)
#define FETCH_METRIC_TX_EXEC_STARTED_EX(hash, timepoint)      \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::EXECUTION_STARTED, timepoint)
#define FETCH_METRIC_TX_EXEC_COMPLETE_EX(hash, timepoint)     \
  fetch::ledger::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::ledger::Metrics::Event::EXECUTION_COMPLETE, timepoint)
#else
#define FETCH_METRIC_TX_SUBMITTED(hash)
#define FETCH_METRIC_TX_QUEUED(hash)
#define FETCH_METRIC_TX_STORED(hash)
#define FETCH_METRIC_TX_PACKED(hash)
#define FETCH_METRIC_TX_EXEC_STARTED(hash)
#define FETCH_METRIC_TX_EXEC_COMPLETE(hash)

#define FETCH_METRIC_TX_SUBMITTED_EX(hash, timepoint)
#define FETCH_METRIC_TX_QUEUED_EX(hash, timepoint)
#define FETCH_METRIC_TX_PACKED_EX(hash, timepoint)
#define FETCH_METRIC_TX_EXEC_STARTED_EX(hash, timepoint)
#define FETCH_METRIC_TX_EXEC_COMPLETE_EX(hash, timepoint)
#endif
