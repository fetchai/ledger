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

#include "metrics/metrics.hpp"
#include "metrics/metric_file_handler.hpp"

namespace fetch {
namespace metrics {

Metrics::Metrics() = default;

Metrics::~Metrics()
{
  handler_.store(nullptr);
  handler_object_.reset();
}

Metrics &Metrics::Instance()
{
  static Metrics instance;
  return instance;
}

void Metrics::ConfigureFileHandler(std::string filename)
{
  std::unique_ptr<MetricHandler> new_handler(new MetricFileHandler(std::move(filename)));

  handler_.store(new_handler.get());
  handler_object_ = std::move(new_handler);
}

void Metrics::RecordMetric(ConstByteArray const &identifier, Instrument instrument, Event event,
                           Timestamp const &timestamp)
{
  auto handler = handler_.load();
  if (handler)
  {
    handler->RecordMetric(identifier, instrument, event, timestamp);
  }
}

void Metrics::RecordTransactionMetric(ConstByteArray const &hash, Event event,
                                      Timestamp const &timestamp)
{
  RecordMetric(hash, Instrument::TRANSACTION, event, timestamp);
}

void Metrics::RecordBlockMetric(ConstByteArray const &hash, Event event, Timestamp const &timestamp)
{
  RecordMetric(hash, Instrument::BLOCK, event, timestamp);
}

}  // namespace metrics
}  // namespace fetch
