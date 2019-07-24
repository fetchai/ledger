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

#include <chrono>

namespace fetch {
namespace telemetry {

class Histogram;

class FunctionTimer
{
public:
  // Construction / Destruction
  explicit FunctionTimer(Histogram &histogram);
  FunctionTimer(FunctionTimer const &) = delete;
  FunctionTimer(FunctionTimer &&)      = delete;
  ~FunctionTimer();

  // Operators
  FunctionTimer &operator=(FunctionTimer const &) = delete;
  FunctionTimer &operator=(FunctionTimer &&) = delete;

private:
  using Clock     = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;

  Histogram &histogram_;
  Timepoint  started_;
};

}  // namespace telemetry
}  // namespace fetch
