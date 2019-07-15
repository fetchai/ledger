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

#include "core/string/ends_with.hpp"
#include "telemetry/measurement.hpp"

#include <iomanip>
#include <iostream>
#include <mutex>
#include <type_traits>

namespace fetch {
namespace telemetry {

/**
 * Gauge Telemetry values
 *
 * The gauge value stores a metric value that is expected to go up and down
 *
 * @tparam ValueType
 */
template <typename ValueType>
class Gauge : public Measurement
{
public:
  // Construction / Destruction
  explicit Gauge(std::string const &name, std::string const &description,
                 Labels const &labels = Labels{});
  Gauge(Gauge const &) = delete;
  Gauge(Gauge &&)      = delete;
  ~Gauge() override    = default;

  /// @name Accessors
  /// @{
  ValueType get() const;
  void      set(ValueType const &value);
  void      increment(ValueType const &value = ValueType{1});
  void      decrement(ValueType const &value = ValueType{1});
  void      max(ValueType const &value);
  /// @}

  /// @name Metric Interface
  /// @{
  void ToStream(std::ostream &stream, StreamMode mode) const override;
  /// @}

  // Operators
  Gauge &operator=(Gauge const &) = delete;
  Gauge &operator=(Gauge &&) = delete;

private:
  using Mutex     = std::mutex;
  using LockGuard = std::lock_guard<Mutex>;

  mutable Mutex lock_{};
  ValueType     value_{0};

  static_assert(std::is_arithmetic<ValueType>::value, "");
};

/**
 * Create a gauge measurement
 *
 * @tparam V The underlying gauge type
 * @param name The name of the gauge. Must not end in '_count'
 * @param description The description of the gauge
 * @param labels The labels for the gauge
 */
template <typename V>
Gauge<V>::Gauge(std::string const &name, std::string const &description, Labels const &labels)
  : Measurement(name, description, labels)
{
  // validate the name
  if (core::EndsWith(this->name(), "_count"))
  {
    throw std::runtime_error("Incorrect name for the gauge, can't end with '_count'");
  }
}

/**
 * Get the current value of the gauge
 *
 * @tparam V The underlying gauge type
 * @return The current value of the gauge
 */
template <typename V>
V Gauge<V>::get() const
{
  LockGuard guard{lock_};
  return value_;
}

/**
 * Sets the value of the gauge to the value specified
 *
 * @tparam V The underlying gauge type
 * @param value The new value to be set
 */
template <typename V>
void Gauge<V>::set(V const &value)
{
  LockGuard guard{lock_};
  value_ = value;
}

/**
 * Increment the value of the gauge by a specified amount
 *
 * @tparam V The underlying gauge type
 */
template <typename V>
void Gauge<V>::increment(V const &value)
{
  LockGuard guard{lock_};
  value_ += value;
}

/**
 * Decrement the value fo the gauge by a specified amount
 *
 * @tparam V The underlying gauge type
 */
template <typename V>
void Gauge<V>::decrement(V const &value)
{
  LockGuard guard{lock_};
  value_ -= value;
}

/**
 * Update the value of the gauge if the input value is bigger than the previous entry
 *
 * @tparam V The underlying gauge type
 * @param value The value to be compared
 */
template <typename V>
void Gauge<V>::max(V const &value)
{
  LockGuard guard{lock_};
  if (value > value_)
  {
    value_ = value;
  }
}

/**
 * Internal: int8_t specific value formatter
 *
 * @param gauge The reference to the current gauge
 * @param stream The stream to be populated
 */
inline void GaugeToStream(Gauge<int8_t> const &gauge, std::ostream &stream)
{
  stream << static_cast<int32_t>(gauge.get()) << '\n';
}

/**
 * Internal: uint8_t specific value formatter
 *
 * @param gauge The reference to the current gauge
 * @param stream The stream to be populated
 */
inline void GaugeToStream(Gauge<uint8_t> const &gauge, std::ostream &stream)
{
  stream << static_cast<uint32_t>(gauge.get()) << '\n';
}

/**
 * Internal: integer value formatter
 *
 * @param gauge The reference to the current gauge
 * @param stream The stream to be populated
 */
template <typename V>
typename std::enable_if<std::is_integral<V>::value>::type GaugeToStream(Gauge<V> const &gauge,
                                                                        std::ostream &  stream)
{
  stream << gauge.get() << '\n';
}

/**
 * Internal: float value formatter
 *
 * @param gauge The reference to the current gauge
 * @param stream The stream to be populated
 */
template <typename V>
typename std::enable_if<std::is_floating_point<V>::value>::type GaugeToStream(Gauge<V> const &gauge,
                                                                              std::ostream &stream)
{
  stream << std::scientific << gauge.get() << '\n';
}

/**
 * Write the value of the metric to the stream so as to be consumed by external components
 *
 * @param stream The stream to be updated
 * @param mode The mode to be used when generating the stream
 */
template <typename V>
void Gauge<V>::ToStream(std::ostream &stream, StreamMode mode) const
{
  WriteHeader(stream, "gauge", mode);
  WriteValuePrefix(stream);
  GaugeToStream(*this, stream);
}

}  // namespace telemetry
}  // namespace fetch
