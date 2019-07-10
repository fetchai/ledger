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
#include "telemetry/telemetry.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace fetch {
namespace telemetry {

class HistogramMap : public Measurement
{
public:
  // Construction / Destruction
  HistogramMap(std::string const &name, std::string field, std::vector<double> buckets,
               std::string const &description, Labels const &labels = Labels{});
  HistogramMap(HistogramMap const &) = delete;
  HistogramMap(HistogramMap &&)      = delete;
  ~HistogramMap() override           = default;

  /// @name Accessors
  /// @{
  void Add(std::string const &key, double const &value);
  /// @}

  /// @name Measurement Interface
  /// @{
  void ToStream(std::ostream &stream, StreamMode mode) const override;
  /// @}

  // Operators
  HistogramMap &operator=(HistogramMap const &) = delete;
  HistogramMap &operator=(HistogramMap &&) = delete;

private:
  using Mutex               = std::mutex;
  using LockGuard           = std::lock_guard<Mutex>;
  using HistogramCollection = std::unordered_map<std::string, HistogramPtr>;

  HistogramPtr LookupHistogram(std::string const &key);

  std::string const         field_;
  std::vector<double> const buckets_;

  mutable Mutex       lock_;
  HistogramCollection histograms_;
};

}  // namespace telemetry
}  // namespace fetch
