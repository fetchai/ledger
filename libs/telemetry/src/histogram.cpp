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

#include <iostream>

namespace fetch {
namespace telemetry {

/**
 * Create a histogram from a init. list of bucket values
 *
 * @param buckets The init. list of bucket values to be used
 * @param name The name of the metric
 * @param description The description of the metric
 * @param labels The labels associated with the metric
 */
Histogram::Histogram(std::initializer_list<double> const &buckets, std::string const &name,
                     std::string const &description, Labels const &labels)
  : Histogram(buckets.begin(), buckets.end(), name, description, labels)
{}

/**
 * Create a histogram with a vector of bucket values
 *
 * @param buckets The vector of bucket values to be used
 * @param name The name of the metric
 * @param description The description of the metric
 * @param labels The labels associated with the metric
 */
Histogram::Histogram(std::vector<double> const &buckets, std::string const &name,
                     std::string const &description, Labels const &labels)
  : Histogram(buckets.begin(), buckets.end(), name, description, labels)
{}

/**
 * Internal: Main constructor for the object
 *
 * @tparam Iterator The iterator type for the bucket list
 * @param begin The iterator to the beginning of the bucket list
 * @param end The iterator to the end of the bucket list
 * @param name The name of the metric
 * @param description The description of the metric
 * @param labels The labels associated with the metric
 */
template <typename Iterator>
Histogram::Histogram(Iterator const &begin, Iterator const &end, std::string const &name,
    std::string const &description, Labels const &labels)
    : Measurement{name, description, labels}
{
  LockGuard guard{lock_};

  // build up the initial bucket values
  for (auto it = begin; it != end; ++it)
  {
    buckets_.emplace(*it, 0u);
  }
}

/**
 * Add a value to the histogram
 *
 * @param value The value to be added
 */
void Histogram::Add(double const &value)
{
  LockGuard guard{lock_};

  // update all of the buckets
  for (auto it = buckets_.lower_bound(value), end = buckets_.end(); it != end; ++it)
  {
    ++(it->second);
  }

  // update the aggregates
  ++count_;
  sum_ += value;
}

/**
 * Write the value of the metric to the stream so as to be consumed by external components
 *
 * @param stream The stream to be updated
 * @param mode The mode to be used when generating the stream
 */
void Histogram::ToStream(std::ostream &stream, StreamMode mode) const
{
  LockGuard guard{lock_};

  WriteHeader(stream, "histogram", mode);
  for (auto const &element : buckets_)
  {
    WriteValuePrefix(stream, "bucket", {{"le", std::to_string(element.first) }}) << element.second << '\n';
  }
  WriteValuePrefix(stream, "bucket", {{"le", "+Inf"}}) << count_ << '\n';

  WriteValuePrefix(stream, "sum") << sum_ << '\n';
  WriteValuePrefix(stream, "count") << count_ << '\n';
}

} // namespace telemetry
} // namespace fetch
