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

#include "core/mutex.hpp"
#include "meta/containers/set_element.hpp"
#include "telemetry/measurement.hpp"
#include "telemetry/telemetry.hpp"

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
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
  using Measurements =
      std::unordered_map<std::string, std::unordered_map<std::type_index, MeasurementPtr>>;

  // Construction / Destruction
  Registry()  = default;
  ~Registry() = default;

  static bool ValidateName(std::string const &name);

  template <class M, class... Args>
  std::shared_ptr<M> Insert(std::string const &name, Args &&... args)
  {
    FETCH_LOCK(lock_);

    auto &          named_cell = measurements_[name];
    std::type_index type{typeid(M)};
    auto            cell_it = named_cell.find(type);
    if (cell_it == named_cell.end())
    {
      cell_it = named_cell.emplace(type, std::make_shared<M>(std::forward<Args>(args)...)).first;
    }

    auto ret_val = std::dynamic_pointer_cast<M>(cell_it->second);
    assert(ret_val);
    return ret_val;
  }

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
  if (!ValidateName(name))
  {
    return {};
  }

  return Insert<Gauge<T>>(name, std::move(name), std::move(description), std::move(labels));
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
  FETCH_LOCK(lock_);
  auto named_cell = measurements_.find(name);
  if (named_cell == measurements_.end())
  {
    return {};
  }

  return core::Lookup(named_cell->second, std::type_index(typeid(T)));
}

}  // namespace telemetry
}  // namespace fetch
