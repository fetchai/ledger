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

#include <atomic>
#include <string>

namespace fetch {
namespace telemetry {

class Counter : public Measurement
{
public:
  // Construction / Destruction
  explicit Counter(std::string name, std::string description, Labels labels = Labels{});
  Counter(Counter const &) = delete;
  Counter(Counter &&)      = delete;
  ~Counter() override      = default;

  /// @name Accessors
  /// @{
  uint64_t count() const;
  void     increment();
  void     add(uint64_t value);
  /// @}

  /// @name Metric Interface
  /// @{
  void ToStream(OutputStream &stream) const override;
  /// @}

  // Operators
  Counter &operator++();
  Counter &operator+=(uint64_t value);
  Counter &operator=(Counter const &) = delete;
  Counter &operator=(Counter &&) = delete;

private:
  std::atomic<uint64_t> counter_{0};
};

}  // namespace telemetry
}  // namespace fetch
