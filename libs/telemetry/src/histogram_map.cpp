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

#include "telemetry/histogram.hpp"
#include "telemetry/histogram_map.hpp"

namespace fetch {
namespace telemetry {

/**
 * Creates a new instance of the histogram map
 *
 * @param name The name of the metric
 * @param field The identifying field for the metric to map against
 * @param buckets The list of bucket values to be used
 * @param description The description of the metric
 * @param labels The labels associated with the metric
 */
HistogramMap::HistogramMap(std::string const &name, std::string field, std::vector<double> buckets,
                           std::string const &description, Labels const &labels)
  : Measurement(name, description, labels)
  , field_{std::move(field)}
  , buckets_{std::move(buckets)}
{
}

/**
 * Add a value with the specified key to the histogram map
 *
 * @param key The identifying key
 * @param value The value to add
 */
void HistogramMap::Add(std::string const &key, double const &value)
{
  LookupHistogram(key)->Add(value);
}

/**
 * Write the value of the metric to the stream so as to be consumed by external components
 *
 * @param stream The stream to be updated
 * @param mode The mode to be used when generating the stream
 */
void HistogramMap::ToStream(std::ostream &stream, StreamMode mode) const
{
  LockGuard guard{lock_};
  WriteHeader(stream, "histogram", mode);

  for (auto const &e : histograms_)
  {
    e.second->ToStream(stream, StreamMode::WITHOUT_HEADER);
  }
}

/**
 * Internal: Lookup or create a new histogram for the specified key
 *
 * @param key The key being queried
 * @return The existing or created histogram
 */
HistogramPtr HistogramMap::LookupHistogram(std::string const &key)
{
  LockGuard guard{lock_};
  auto it = histograms_.find(key);
  if (it == histograms_.end())
  {
    // create and update the set of labels
    Labels new_labels = labels();
    new_labels[field_] = key;

    // create a new histogram
    auto histogram = std::make_shared<Histogram>(buckets_, name(), "", new_labels);

    // update the map
    histograms_[key] = histogram;

    return histogram;
  }
  else
  {
    // return the existing histogram
    return it->second;
  }
}

} // namespace telemetry
} // namespace fetch
