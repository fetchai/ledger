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

#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace fetch {
namespace core {

/**
 * Semaphore like synchronization object
 */
class Tickets
{
public:
  // Construction / Destruction
  Tickets(std::size_t initial = 0);
  Tickets(Tickets const &) = delete;
  Tickets(Tickets &&)      = delete;
  ~Tickets();

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
  std::atomic<bool>       shutdown_{false};
};

inline Tickets::Tickets(std::size_t initial)
  : count_{initial}
{}

inline Tickets::~Tickets()
{
  shutdown_ = true;
  cv_.notify_all();
}

/**
 * Post / increment the internal counter
 */
inline void Tickets::Post()
{
  std::lock_guard<std::mutex> lock(mutex_);
  if (++count_)
  {
    cv_.notify_one();
  }
}

/**
 * Wait / decrement the internal counter
 *
 * This function will block until a ticket can be claimed
 */
inline void Tickets::Wait()
{
  std::unique_lock<std::mutex> lock(mutex_);

  for (;;)
  {
    if (shutdown_)
    {
      return;
    }

    // check an update the count
    if (count_ > 0)
    {
      --count_;
      break;
    }

    // wait for an event to be triggered
    cv_.wait(lock);
  }
}

/**
 * Wait / decrement the internal counter
 *
 * This function will block maximally for the specified duration
 *
 * @tparam R The representation of the duration
 * @tparam P The period for the duration
 * @param duration The maximum duration to wait for the ticket
 * @return true if a ticket was acquired, otherwise false
 */
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
      {
        return false;
      }

      if (count_ == 0)
      {
        Timepoint now = Clock::now();

        // detect if we have exceeded the deadline
        if (now >= deadline)
        {
          break;
        }

        // otherwise configure the CV wait for the remaining time
        if (std::cv_status::timeout == cv_.wait_until(lock, deadline))
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
