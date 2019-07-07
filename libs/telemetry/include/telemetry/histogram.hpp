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

#include <initializer_list>
#include <map>
#include <mutex>
#include <functional>
#include <vector>
#include <string>

namespace fetch {
namespace telemetry {

class Histogram : public Measurement
{
public:

  // Construction / Destruction
  Histogram(std::initializer_list<double> const &buckets, std::string const &name,
            std::string const &description, Labels const &labels = Labels{});
  Histogram(std::vector<double> const &buckets, std::string const &name,
            std::string const &description, Labels const &labels = Labels{});
  Histogram(Histogram const &) = delete;
  Histogram(Histogram &&) = delete;
  ~Histogram() override = default;

  /// @name Accessors
  /// @{
  void Add(double const &value);
  /// @}

  /// @name Measurement Interface
  /// @{
  void ToStream(std::ostream &stream, StreamMode mode) const override;
  /// @}

  // Operators
  Histogram &operator=(Histogram const &) = delete;
  Histogram &operator=(Histogram &&) = delete;

private:
  using Mutex     = std::mutex;
  using LockGuard = std::lock_guard<Mutex>;
  using BucketMap = std::map<double, uint64_t>;

  template <typename Iterator>
  Histogram(Iterator const &begin, Iterator const &end, std::string const &name,
            std::string const &description, Labels const &labels = Labels{});

  mutable Mutex lock_;
  BucketMap     buckets_;
  uint64_t      count_{0};
  double        sum_{0.0};
};



} // namespace telemetry
} // namespace fetch
