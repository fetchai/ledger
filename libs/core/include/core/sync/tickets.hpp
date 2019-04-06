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
  using Count = std::size_t;

  // Construction / Destruction
  Tickets(Count initial = 0);
  Tickets(Tickets const &) = delete;
  Tickets(Tickets &&)      = delete;

  void Post();
  void Post(Count &count);

  void Wait();
  template <typename R, typename P>
  bool Wait(std::chrono::duration<R, P> const &duration);

  std::size_t size();

  // Operators
  Tickets &operator=(Tickets const &) = delete;
  Tickets &operator=(Tickets &&) = delete;

private:
  std::mutex              mutex_;
  std::condition_variable cv_;
  Count                   count_;
};

inline Tickets::Tickets(Count initial)
  : count_{initial}
{}

/**
 * Post / increment the internal counter
 */
inline void Tickets::Post()
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    ++count_;
  }
  cv_.notify_one();
}

/**
 * Post / increment the internal counter
 * @param count - current number of submitted tickets (=number of tickets still in waiting stage)
 */
inline void Tickets::Post(Count &count)
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    count = ++count_;
  }
  cv_.notify_one();
}

/**
 * Wait / decrement the internal counter
 *
 * This function will block until a ticket can be claimed
 */
inline void Tickets::Wait()
{
  std::unique_lock<std::mutex> lock(mutex_);
  // wait for an event to be triggered
  while (count_ == 0)
  {
    cv_.wait(lock);
  }
  --count_;
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

  {
    std::unique_lock<std::mutex> lock(mutex_);

    // loop required since it is possible because we are emulating the semaphore behaviour that
    // even though we were triggered by the CV another worker might have take our place.
    while (count_ == 0)
    {
      // detect if we have exceeded the deadline
      if (std::cv_status::timeout == cv_.wait_until(lock, deadline))
      {
        return false;
      }
    }
    --count_;
    return true;
  }
}

}  // namespace core
}  // namespace fetch
