#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace fetch {
namespace core {

/**
 * Semaphore like object
 */
class Tickets
{
public:
  // Construction / Destruction
  Tickets(std::size_t initial = 0)
    : count_{initial}
  {}

  Tickets(Tickets const &) = delete;
  Tickets(Tickets &&)      = delete;
  ~Tickets()               = default;

  void Post();
  void Wait();

  template <typename R, typename P>
  bool Wait(std::chrono::duration<R, P> const &duration);

  // Operators
  Tickets &operator=(Tickets const &) = delete;
  Tickets &operator=(Tickets &&) = delete;

private:
  std::mutex              mutex_;
  std::condition_variable cv_;
  std::size_t             count_;
  bool                    shutdown_{false};
};

inline void Tickets::Post()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (++count_)
  {
    cv_.notify_one();
  }
}

inline void Tickets::Wait()
{
  std::unique_lock<std::mutex> lock(mutex_);

  if (shutdown_)
    return;

  if (count_ == 0)
  {
    cv_.wait(lock);
  }

  if (shutdown_)
    return;

  assert(count_ > 0);
  --count_;
}

template <typename R, typename P>
bool Tickets::Wait(std::chrono::duration<R, P> const &duration)
{
  using Clock     = std::chrono::high_resolution_clock;
  using Timepoint = Clock::time_point;

  // calculate the deadline
  Timepoint deadline = Clock::now() + duration;

  bool success = false;

  {
    std::unique_lock<std::mutex> lock(mutex_);

    // loop required since it is possible because we are emulating the semaphore behaviour that
    // even though we were triggered by the CV another worker might have take our place.
    for (;;)
    {
      if (shutdown_)
        return false;

      if (count_ == 0)
      {
        Timepoint now = Clock::now();

        // detect if we have exceeded the deadline
        if (now >= deadline)
        {
          break;
        }

        // otherwise configure the CV wait for the remaining time
        if (std::cv_status::timeout == cv_.wait_for(lock, deadline - now))
        {
          break;
        }
      }
      else
      {
        --count_;
        success = true;
        break;
      }
    }
  }

  return success;
}

}  // namespace core
}  // namespace fetch
