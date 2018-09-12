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

#include "ledger/metrics/metrics.hpp"
#include "ledger/metrics/metric_file_handler.hpp"

namespace fetch {
namespace ledger {

Metrics &Metrics::Instance()
{
  static Metrics instance;
  return instance;
}

void Metrics::ConfigureFileHandler(std::string filename)
{
  SetMetricHandler(new MetricFileHandler(std::move(filename)));
}

void Metrics::SetMetricHandler(MetricHandler* handler)
{
  MetricHandler* old_handler = handler_.exchange(handler);
  if (old_handler)
  {
    delete old_handler;
  }
}

} // namespace ledger
} // namespace fetch
