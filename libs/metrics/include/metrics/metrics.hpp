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

#include "metrics/metric_handler.hpp"

#include <string>

namespace fetch {
namespace metrics {

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

  Metrics(Metrics const &) = delete;
  Metrics(Metrics &&)      = delete;

  // Configuration
  void ConfigureFileHandler(std::string filename);

  void RecordMetric(ConstByteArray const &identifier, Instrument instrument, Event event,
                    Timestamp const &timestamp = Clock::now());

  void RecordTransactionMetric(ConstByteArray const &hash, Event event,
                               Timestamp const &timestamp = Clock::now());

  void RecordBlockMetric(ConstByteArray const &hash, Event event,
                         Timestamp const &timestamp = Clock::now());

  // Operators
  Metrics &operator=(Metrics const &) = delete;
  Metrics &operator=(Metrics &&) = delete;

private:
  Metrics();
  ~Metrics();

  std::unique_ptr<MetricHandler> handler_object_;
  std::atomic<MetricHandler *>   handler_{nullptr};
};

}  // namespace metrics
}  // namespace fetch

#ifdef FETCH_ENABLE_METRICS
#define FETCH_METRIC_TX_SUBMITTED(hash)                        \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::SUBMITTED)
#define FETCH_METRIC_TX_STORED(hash)                           \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::STORED)
#define FETCH_METRIC_TX_QUEUED(hash)                           \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::QUEUED)
#define FETCH_METRIC_TX_PACKED(hash)                           \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::PACKED)
#define FETCH_METRIC_TX_EXEC_STARTED(hash)                     \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::EXECUTION_STARTED)
#define FETCH_METRIC_TX_EXEC_COMPLETE(hash)                    \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::EXECUTION_COMPLETE)

#define FETCH_METRIC_TX_SUBMITTED_EX(hash, timestamp)          \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::SUBMITTED, timestamp)
#define FETCH_METRIC_TX_STORED_EX(hash, timestamp)             \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::STORED, timestamp)
#define FETCH_METRIC_TX_QUEUED_EX(hash, timestamp)             \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::QUEUED, timestamp)
#define FETCH_METRIC_TX_PACKED_EX(hash, timestamp)             \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::PACKED, timestamp)
#define FETCH_METRIC_TX_EXEC_STARTED_EX(hash, timestamp)       \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::EXECUTION_STARTED, timestamp)
#define FETCH_METRIC_TX_EXEC_COMPLETE_EX(hash, timestamp)      \
  fetch::metrics::Metrics::Instance().RecordTransactionMetric( \
      hash, fetch::metrics::Metrics::Event::EXECUTION_COMPLETE, timestamp)

#define FETCH_METRIC_BLOCK_GENERATED(hash)                    \
  fetch::metrics::Metrics::Instance().RecordBlockMetric(hash, \
                                                        fetch::metrics::Metrics::Event::GENERATED)

#define FETCH_METRIC_BLOCK_RECEIVED(hash)                     \
  fetch::metrics::Metrics::Instance().RecordBlockMetric(hash, \
                                                        fetch::metrics::Metrics::Event::RECEIVED)

#else  // !FETCH_ENABLE_METRICS

#define FETCH_METRIC_TX_SUBMITTED(hash)
#define FETCH_METRIC_TX_STORED(hash)
#define FETCH_METRIC_TX_QUEUED(hash)
#define FETCH_METRIC_TX_PACKED(hash)
#define FETCH_METRIC_TX_EXEC_STARTED(hash)
#define FETCH_METRIC_TX_EXEC_COMPLETE(hash)

#define FETCH_METRIC_TX_SUBMITTED_EX(hash, timestamp)
#define FETCH_METRIC_TX_STORED_EX(hash, timestamp)
#define FETCH_METRIC_TX_QUEUED_EX(hash, timestamp)
#define FETCH_METRIC_TX_PACKED_EX(hash, timestamp)
#define FETCH_METRIC_TX_EXEC_STARTED_EX(hash, timestamp)
#define FETCH_METRIC_TX_EXEC_COMPLETE_EX(hash, timestamp)

#define FETCH_METRIC_BLOCK_GENERATED(hash)
#define FETCH_METRIC_BLOCK_RECEIVED(hash)

#endif  // FETCH_ENABLE_METRICS
