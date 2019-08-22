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

#include "core/logger.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

namespace fetch {
namespace generics {

class MilliTimer
{
public:
  using Clock     = std::chrono::steady_clock;
  using Timepoint = Clock::time_point;

  static constexpr char const *LOGGING_NAME = "MilliTimer";

  MilliTimer(MilliTimer const &rhs) = delete;
  MilliTimer(MilliTimer &&rhs)      = delete;
  MilliTimer &operator=(MilliTimer const &rhs) = delete;
  MilliTimer &operator=(MilliTimer &&rhs) = delete;

  explicit MilliTimer(std::string name, int64_t threshold = 100)
    : start_(Clock::now())
    , name_(std::move(name))
    , threshold_(threshold)
  {
    if (!threshold)
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Starting millitimer for ", name_);
    }
  }

  ~MilliTimer()
  {
    auto const duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start_);

    if (duration.count() > threshold_)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Timer: ", name_, " duration: ", duration.count(), "ms");
    }
    else
    {
      FETCH_LOG_DEBUG(LOGGING_NAME, "Consumed milliseconds: ", duration.count(), " at ", name_);
    }
  }

private:
  // members here.

  Timepoint   start_;
  std::string name_;
  int64_t     threshold_;
};

}  // namespace generics
}  // namespace fetch
