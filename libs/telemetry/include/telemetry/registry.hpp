#pragma once
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

#include "telemetry/measurement.hpp"
#include "telemetry/telemetry.hpp"

#include <cassert>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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

  HistogramPtr CreateHistogram(std::initializer_list<double> buckets, std::string name,
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

  class FastMeasurementHash
  {
    static constexpr char KEY   = '\0';
    static constexpr char VALUE = '\1';

  public:
    std::size_t operator()(MeasurementPtr const &measurement) const;
  };

  struct LabelsEqual
  {
    bool operator()(MeasurementPtr const &left, MeasurementPtr const &right) const;
  };

  using SameNameMeasurements = std::unordered_set<MeasurementPtr, FastMeasurementHash, LabelsEqual>;
  using Measurements         = std::unordered_map<std::string, SameNameMeasurements>;

  // Construction / Destruction
  Registry()  = default;
  ~Registry() = default;

  template <class M, class... Args>
  std::shared_ptr<M> Create(std::string name, Args &&... args);

  static bool ValidateName(std::string const &name);

  template <class M>
  std::shared_ptr<M> Insert(std::string const &name, std::shared_ptr<M> m);

  mutable std::mutex lock_;
  Measurements       measurements_;
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
  return Create<Gauge<T>>(name, std::move(name), std::move(description), std::move(labels));
}

/**
 * Look up an existing metric from the registry
 *
 * @tparam T The underlying metric type being requested
 * @param name The name of the metric
 * @return A metric matching the name, otherwise a null shared pointer
 */
template <typename T>
std::shared_ptr<T> Registry::LookupMeasurement(std::string const &name) const
{
  std::lock_guard<std::mutex> guard(lock_);
  auto                        measurements_it = measurements_.find(name);
  if (measurements_it == measurements_.end())
  {
    return {};
  }

  auto const &named_cell = measurements_it->second;
  assert(!named_cell.empty());
  return std::dynamic_pointer_cast<T>(*named_cell.begin());
}

template <class M, class... Args>
std::shared_ptr<M> Registry::Create(std::string name, Args &&... args)
{
  if (!ValidateName(name))
  {
    return {};
  }

  return Insert(name, std::make_shared<M>(std::forward<Args>(args)...));
}

template <class M>
std::shared_ptr<M> Registry::Insert(std::string const &name, std::shared_ptr<M> m)
{
  std::lock_guard<std::mutex> guard(lock_);

  auto &named_cell = measurements_[name];

  auto cell_it = named_cell.insert(std::move(m)).first;

  return std::dynamic_pointer_cast<M>(*cell_it);
}

}  // namespace telemetry
}  // namespace fetch
