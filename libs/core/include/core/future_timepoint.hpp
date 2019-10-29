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
#include <thread>

namespace fetch {
namespace core {

/**
 * Simple wrapper around std time classes to express a future planned time.
 */
class FutureTimepoint
{
public:
  using Clock     = std::chrono::steady_clock;
  using Duration  = Clock::duration;
  using Timepoint = Clock::time_point;

  explicit FutureTimepoint(Duration const &dur)
  {
    due_time_ = Clock::now() + dur;
  }

  FutureTimepoint()
  {
    due_time_ = Clock::now() - std::chrono::seconds(10000);
  }

  ~FutureTimepoint() = default;

  void SetSeconds(std::size_t seconds)
  {
    due_time_ = Clock::now() + std::chrono::seconds(seconds);
  }

  void SetMilliseconds(std::size_t milliseconds)
  {
    due_time_ = Clock::now() + std::chrono::milliseconds(milliseconds);
  }

  void Set(Duration const &dur)
  {
    due_time_ = Clock::now() + dur;
  }

  void SetTimedOut()
  {
    due_time_ = Clock::now() - std::chrono::seconds(1);
  }

  void SetMilliseconds(Timepoint const &timepoint, std::size_t milliseconds)
  {
    due_time_ = timepoint + std::chrono::milliseconds(milliseconds);
  }

  FutureTimepoint &operator=(std::size_t milliseconds)
  {
    due_time_ = Clock::now() + std::chrono::milliseconds(milliseconds);
    return *this;
  }

  FutureTimepoint &operator=(Duration dur)
  {
    due_time_ = Clock::now() + dur;
    return *this;
  }

  bool IsDue(Timepoint const &time_point) const
  {
    return due_time_ <= time_point;
  }

  bool IsDue() const
  {
    return IsDue(Clock::now());
  }

  Duration DueIn() const
  {
    return due_time_ - Clock::now();
  }

  void WaitFor()
  {
    std::this_thread::sleep_for(DueIn());
  }

  std::string Explain() const
  {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    auto const due_in_ms = duration_cast<milliseconds>(DueIn()).count();
    return std::to_string(due_in_ms) + "ms";
  }

private:
  Timepoint due_time_;
};

}  // namespace core
}  // namespace fetch
