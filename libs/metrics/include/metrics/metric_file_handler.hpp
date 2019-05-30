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

#include "core/mutex.hpp"
#include "metrics/metric_handler.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

namespace fetch {
namespace metrics {

/**
 * A Metric Handler that outputs recorded metrics to a CSV file
 */
class MetricFileHandler : public MetricHandler
{
public:
  // Construction / Destruction
  explicit MetricFileHandler(std::string filename);
  MetricFileHandler(MetricHandler const &) = delete;
  MetricFileHandler(MetricHandler &&)      = delete;
  ~MetricFileHandler() override;

  /// @name Metric Handler Interface
  /// @{
  void RecordMetric(ConstByteArray const &identifier, Instrument instrument, Event event,
                    Timestamp const &timestamp) override;
  /// @}

  // Operators
  MetricFileHandler &operator=(MetricFileHandler const &) = delete;
  MetricFileHandler &operator=(MetricFileHandler &&) = delete;

private:
  static constexpr std::size_t BUFFER_SIZE = 1 << 18u;  // 262144

  struct Entry
  {
    ConstByteArray identifier;
    Instrument     instrument;
    Event          event;
    Timestamp      timestamp;
  };

  using Mutex      = mutex::Mutex;
  using EntryStack = std::vector<Entry>;

  void ThreadEntryPoint();

  std::string const       filename_;  ///< The filename for the output file
  Mutex                   stack_lock_{__LINE__, __FILE__};
  std::condition_variable stack_notify_;  ///< The notification variable for
  EntryStack              stack_;         ///< The stack of events that need to be generated
  std::atomic<bool>       active_;        ///< Active monitor thread
  std::thread             worker_;        ///< The worker thread
};

}  // namespace metrics
}  // namespace fetch
