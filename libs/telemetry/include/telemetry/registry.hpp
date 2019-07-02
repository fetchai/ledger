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

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace telemetry {

class Measurement;
class Counter;

template <typename T>
class Gauge;

/**
 * Global registry of metrics across the system
 */
class Registry
{
public:
  using Labels = std::unordered_map<std::string, std::string>;

  template <typename T>
  using GaugePtr = std::shared_ptr<Gauge<T>>;

  static Registry &Instance();

  // Construction / Destruction
  Registry()                 = default;
  Registry(Registry const &) = delete;
  Registry(Registry &&)      = delete;
  ~Registry()                = default;

  /// @name Metric Helpers
  /// @{
  CounterPtr CreateCounter(std::string name, std::string description = "",
                           Labels labels = Labels{});

  template <typename T>
  GaugePtr<T> CreateGauge(std::string name, std::string description = "", Labels labels = Labels{});
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

  static bool ValidateName(std::string const &name);

  Mutex        lock_;
  Measurements measurements_;
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

}  // namespace telemetry
}  // namespace fetch
