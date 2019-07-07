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

#include "telemetry/counter.hpp"
#include "telemetry/counter_map.hpp"
#include "telemetry/gauge.hpp"
#include "telemetry/histogram.hpp"
#include "telemetry/histogram_map.hpp"
#include "telemetry/registry.hpp"

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
  bool valid{true};

  for (char c : name)
  {
    valid = (c == '_') || ((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9'));

    // exit as soon as it it not valid
    if (!valid)
    {
      break;
    }
  }

  return valid;
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
  CounterPtr counter{};

  if (ValidateName(name))
  {
    // create the new counter
    counter = std::make_shared<Counter>(std::move(name), std::move(description), std::move(labels));

    // add the counter to the register
    {
      LockGuard guard(lock_);
      measurements_.push_back(counter);
    }
  }

  return counter;
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
  CounterMapPtr map{};

  if (ValidateName(name))
  {
    // create the new counter
    map = std::make_shared<CounterMap>(std::move(name), std::move(description), std::move(labels));

    // add the counter to the register
    {
      LockGuard guard(lock_);
      measurements_.emplace_back(map);
    }
  }

  return map;
}

/**
 * Create a histogram instance
 *
 * @param buckets The set of buckets to be used
 * @param name The name of the metric
 * @param description The description of the metric
 * @param labels The labels assoicated with the metric
 * @return The pointer to the created metric if successful, otherwise a nullptr
 */
HistogramPtr Registry::CreateHistogram(std::initializer_list<double> const &buckets,
                                       std::string name, std::string description, Labels labels)
{
  HistogramPtr histogram{};

  if (ValidateName(name))
  {
    // create the histogram
    histogram = std::make_shared<Histogram>(buckets, name, description, labels);

    // add the counter to the register
    {
      LockGuard guard(lock_);
      measurements_.push_back(histogram);
    }
  }

  return histogram;
}

HistogramMapPtr Registry::CreateHistogramMap(std::vector<double> buckets, std::string name,
                                             std::string field, std::string description,
                                             Labels labels)
{
  HistogramMapPtr histogram_map{};

  if (ValidateName(name))
  {
    // create the histogram
    histogram_map =
        std::make_shared<HistogramMap>(std::move(name), std::move(field), std::move(buckets),
                                       std::move(description), std::move(labels));

    // add the counter to the register
    {
      LockGuard guard(lock_);
      measurements_.emplace_back(histogram_map);
    }
  }

  return histogram_map;
}

/**
 * Collect up all the metrics into a single stream to be presented to the requestor
 *
 * @param stream The reference to the stream to be populated
 */
void Registry::Collect(std::ostream &stream)
{
  LockGuard guard(lock_);
  for (auto const &measurement : measurements_)
  {
    measurement->ToStream(stream, Measurement::StreamMode::FULL);
  }
}

}  // namespace telemetry
}  // namespace fetch
