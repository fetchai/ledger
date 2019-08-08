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

#include "telemetry/telemetry.hpp"

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace telemetry {

/**
 * Global registry of metrics across the system
 */
class Registry
{
public:
  static constexpr char const *LOGGING_NAME = "TeleRegistry";

  template <typename T>
  using GaugePtr = std::shared_ptr<Gauge<T>>;
  using Labels   = std::unordered_map<std::string, std::string>;

  static Registry &Instance();

  // Construction / Destruction
  Registry(Registry const &) = delete;
  Registry(Registry &&)      = delete;

  /// @name Metric Helpers
  /// @{
  CounterPtr CreateCounter(std::string name, std::string description, Labels labels = Labels{});

  CounterMapPtr CreateCounterMap(std::string name, std::string description,
                                 Labels labels = Labels{});

  template <typename T>
  GaugePtr<T> CreateGauge(std::string name, std::string description, Labels labels = Labels{});

  HistogramPtr CreateHistogram(std::initializer_list<double> const &buckets, std::string name,
                               std::string description = "", Labels labels = Labels{});

  HistogramMapPtr CreateHistogramMap(std::vector<double> buckets, std::string name,
                                     std::string field, std::string description,
                                     Labels labels = Labels{});
  /// @}

  /// @name Metric Lookups
  /// @{
  template <typename T>
  std::shared_ptr<T> LookupMeasurement(std::string const &name) const;
  /// @}

  void Collect(std::ostream &stream);

  // Operators
  Registry &operator=(Registry const &) = delete;
  Registry &operator=(Registry &&) = delete;

private:
  using MeasurementPtr = std::shared_ptr<Measurement>;
  using Measurements   = std::vector<MeasurementPtr>;
  using Mutex          = std::mutex;
  using LockGuard      = std::lock_guard<std::mutex>;

  // Construction / Destruction
  Registry()  = default;
  ~Registry() = default;

  static bool ValidateName(std::string const &name);

  mutable Mutex lock_;
  Measurements  measurements_;
};

/**
 * Create a counter instance
 *
 * @tparam T The underlying type of the gauge
 * @param name The name of the metric
 * @param description The description of the metric
 * @param labels The labels associated with the metric
 * @return The pointer to the created metric if successful, otherwise a nullptr
 */
template <typename T>
Registry::GaugePtr<T> Registry::CreateGauge(std::string name, std::string description,
                                            Labels labels)
{
  GaugePtr<T> gauge{};

  if (ValidateName(name))
  {
    gauge = std::make_shared<Gauge<T>>(std::move(name), description, std::move(labels));

    // add the gauge to the register
    {
      LockGuard guard(lock_);
      measurements_.push_back(gauge);
    }
  }

  return gauge;
}

/**
 * Lookup and existing metric from the registry
 *
 * @tparam T The underlying metric type being requested
 * @param name The name of the metric
 * @return A metric matching the name, otherwise a null shared pointer
 */
template <typename T>
std::shared_ptr<T> Registry::LookupMeasurement(std::string const &name) const
{
  std::shared_ptr<T> measurement{};

  auto const matcher = [&name](MeasurementPtr const &m) { return (m->name() == name); };

  LockGuard guard{lock_};

  // attempt to find the first metric matching the name with the type
  for (auto start = measurements_.begin(), end = measurements_.end(); start != end;)
  {
    // attempt to locate the next match
    auto match = std::find_if(start, end, matcher);

    if (match != measurements_.end())
    {
      // attempt to convert the measurement to the chosen type
      measurement = std::dynamic_pointer_cast<T>(*match);
      if (measurement)
      {
        break;
      }

      // otherwise advance the iterator to move to the next element (since the type mismatched)
      ++match;
    }

    // move the next element
    start = match;
  }

  return measurement;
}

}  // namespace telemetry
}  // namespace fetch
