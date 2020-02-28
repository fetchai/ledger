//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "telemetry/counter.hpp"
#include "telemetry/counter_map.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/histogram_map.hpp"
#include "telemetry/registry.hpp"

#include <cctype>
#include <initializer_list>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace fetch {
namespace telemetry {

/**
 * Get reference to singleton instance
 * @return The reference to the registry
 */
Registry &Registry::Instance()
{
  static Registry instance;
  return instance;
}

/**
 * Ensure the name consists of lowercase letters and underscores
 *
 * @param name The name to be checked
 * @return true if valid, otherwise false
 */
bool Registry::ValidateName(std::string const &name)
{
  for (char c : name)
  {
    if (!std::islower(c) && c != '_' && !std::isdigit(c))
    {
      return false;
    }
  }

  return true;
}

/**
 * Create a counter instance
 *
 * @param name The name of the metric
 * @param description The description of the metric
 * @param labels The labels associated with the metric
 * @return The pointer to the created metric if successful, otherwise a nullptr
 */
CounterPtr Registry::CreateCounter(std::string name, std::string description, Labels labels)
{
  if (!ValidateName(name))
  {
    return {};
  }
  return Insert<Counter>(name, std::move(name), std::move(description), std::move(labels));
}

/**
 * Create a counter map instance
 *
 * @param name The name of the metric
 * @param description The description fo the metric
 * @param labels The labels associated with the metric
 * @return The pointer rot the created metric if successful, otherwise a nullptr
 */
CounterMapPtr Registry::CreateCounterMap(std::string name, std::string description, Labels labels)
{
  if (!ValidateName(name))
  {
    return {};
  }
  return Insert<CounterMap>(name, std::move(name), std::move(description), std::move(labels));
}

/**
 * Create a histogram instance
 *
 * @param buckets The set of buckets to be used
 * @param name The name of the metric
 * @param description The description of the metric
 * @param labels The labels associated with the metric
 * @return The pointer to the created metric if successful, otherwise a nullptr
 */
HistogramPtr Registry::CreateHistogram(std::initializer_list<double> const &buckets,
                                       std::string name, std::string description, Labels labels)
{
  if (!ValidateName(name))
  {
    return {};
  }
  return Insert<Histogram>(name, buckets, std::move(name), std::move(description),
                           std::move(labels));
}

HistogramMapPtr Registry::CreateHistogramMap(std::vector<double> buckets, std::string name,
                                             std::string field, std::string description,
                                             Labels labels)
{
  if (!ValidateName(name))
  {
    return {};
  }
  return Insert<HistogramMap>(name, std::move(name), std::move(field), std::move(buckets),
                              std::move(description), std::move(labels));
}

/**
 * Collect up all the metrics into a single stream to be presented to the requestor
 *
 * @param stream The reference to the stream to be populated
 */
void Registry::Collect(std::ostream &stream)
{
  OutputStream telemetry_stream{stream};

  std::lock_guard<std::mutex> guard(lock_);
  for (auto const &named_cell : measurements_)
  {
    for (auto const &measurement : named_cell.second)
    {
      measurement.second->ToStream(telemetry_stream);
    }
  }
}

}  // namespace telemetry
}  // namespace fetch
