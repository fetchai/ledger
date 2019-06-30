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

#include "telemetry/measurement.hpp"

#include <atomic>
#include <iomanip>
#include <iostream>
#include <type_traits>

namespace fetch {
namespace telemetry {

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
  void      increment();
  void      decrement();
  void      add(ValueType const &value);
  void      remove(ValueType const &value);
  void      max(ValueType const &value);
  /// @}

  /// @name Metric Interface
  /// @{
  bool ToStream(std::ostream &stream) const override;
  /// @}

  // Operators
  Gauge &operator=(Gauge const &) = delete;
  Gauge &operator=(Gauge &&) = delete;

private:
  std::atomic<ValueType> value_{0};

  static_assert(std::is_arithmetic<ValueType>::value, "");
};

template <typename V>
Gauge<V>::Gauge(std::string const &name, std::string const &description, Labels const &labels)
  : Measurement(name, description, labels)
{}

template <typename V>
V Gauge<V>::get() const
{
  return value_;
}

template <typename V>
void Gauge<V>::set(V const &value)
{
  value_ = value;
}

template <typename V>
void Gauge<V>::increment()
{
  ++value_;
}

template <typename V>
void Gauge<V>::decrement()
{
  --value_;
}

template <typename V>
void Gauge<V>::add(V const &value)
{
  value_ += value;
}

template <typename V>
void Gauge<V>::remove(V const &value)
{
  value_ -= value;
}

template <typename V>
void Gauge<V>::max(V const &value)
{
  if (value > value_)
  {
    value_ = value;
  }
}

inline void GaugeToStream(Gauge<int8_t> const &gauge, std::ostream &stream)
{
  stream << "# HELP " << gauge.name() << ' ' << gauge.description() << '\n'
         << "# TYPE " << gauge.name() << " gauge\n"
         << gauge.name() << ' ' << static_cast<int32_t>(gauge.get()) << '\n';
}

inline void GaugeToStream(Gauge<uint8_t> const &gauge, std::ostream &stream)
{
  stream << "# HELP " << gauge.name() << ' ' << gauge.description() << '\n'
         << "# TYPE " << gauge.name() << " gauge\n"
         << gauge.name() << ' ' << static_cast<uint32_t>(gauge.get()) << '\n';
}

template <typename V>
typename std::enable_if<std::is_integral<V>::value>::type GaugeToStream(Gauge<V> const &gauge,
                                                                        std::ostream &  stream)
{
  stream << "# HELP " << gauge.name() << ' ' << gauge.description() << '\n'
         << "# TYPE " << gauge.name() << " gauge\n"
         << gauge.name() << ' ' << gauge.get() << '\n';
}

template <typename V>
typename std::enable_if<std::is_floating_point<V>::value>::type GaugeToStream(Gauge<V> const &gauge,
                                                                              std::ostream &stream)
{
  stream << "# HELP " << gauge.name() << ' ' << gauge.description() << '\n'
         << "# TYPE " << gauge.name() << " gauge\n"
         << gauge.name() << ' ' << std::scientific << gauge.get() << '\n';
}

template <typename V>
bool Gauge<V>::ToStream(std::ostream &stream) const
{
  GaugeToStream(*this, stream);
  return true;
}

}  // namespace telemetry
}  // namespace fetch
