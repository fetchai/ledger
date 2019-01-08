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

#include "core/mutex.hpp"

#include <algorithm>
#include <iostream>
#include <string>

namespace fetch {
namespace network {
namespace details {

/**
 * The idle work store is a simple array of work items. Its primary role is to coordinate
 * periodic execution of the items that are stored within it.
 */
class IdleWorkStore
{
public:
  using WorkItem   = std::function<void()>;
  using mutex_type = fetch::mutex::Mutex;
  using lock_type  = std::unique_lock<mutex_type>;

  IdleWorkStore()                         = default;
  IdleWorkStore(const IdleWorkStore &rhs) = delete;
  IdleWorkStore(IdleWorkStore &&rhs)      = delete;

  ~IdleWorkStore()
  {
    shutdown_ = true;

    FETCH_LOCK(mutex_);
    store_.clear();  // remove any pending things
  }

  /**
   * Set the interval for the idle work
   *
   * @param milliseconds The interval in milliseconds
   */
  void SetInterval(std::size_t milliseconds)
  {
    FETCH_LOCK(mutex_);
    interval_ = std::chrono::milliseconds(milliseconds);
  }

  /**
   * Clear the contents of the work store
   */
  void Clear()
  {
    FETCH_LOCK(mutex_);
    store_.clear();
  }

  /**
   * Signal that the idle work store should start to shutdown
   */
  void Abort()
  {
    shutdown_ = true;
  }

  /**
   * Determine if the time has come for the periodic work to be executed
   *
   * @return true if the work should be scheduled, otherwise false
   */
  bool IsDue() const
  {
    bool is_due = false;

    if (mutex_.try_lock())
    {
      is_due = Clock::now() >= (last_run_ + interval_);

      mutex_.unlock();
    }

    return is_due;
  }

  /**
   * Determine the amount of time remaining until the work is required
   *
   * @return milliseconds::max() if the work queue is empty, milliseconds::zero()
   * if the time is already due for execution, otherwise the time in milliseconds remaining
   */
  std::chrono::milliseconds DueIn()
  {
    using std::chrono::milliseconds;
    using std::chrono::duration_cast;

    FETCH_LOCK(mutex_);

    if (store_.empty())
    {
      return std::chrono::milliseconds::max();
    }

    Timestamp const now      = Clock::now();
    Timestamp const next_run = last_run_ + interval_;

    if (next_run > now)
    {
      return duration_cast<milliseconds>(next_run - now);
    }
    else
    {
      return std::chrono::milliseconds::zero();
    }
  }

  /**
   * Visit the contents of the idle work store, executing the specified callback
   * over each element of work
   *
   * @tparam CALLBACK A function like type accepting the signature: void(WorkItem const &)
   * @param visitor The callback function
   * @return The number of processed elements
   */
  template <typename CALLBACK>
  std::size_t Visit(CALLBACK const &visitor) const
  {
    std::size_t num_processed = 0;

    if (mutex_.try_lock())
    {
      for (auto const &work : store_)
      {
        // allow early exit
        if (shutdown_)
        {
          break;
        }

        visitor(work);
        ++num_processed;
      }

      // update the last run time
      last_run_ = Clock::now();

      mutex_.unlock();
    }

    return num_processed;
  }

  /**
   * Add a given work item to the queue
   *
   * @param item The item to be stored in the queue
   */
  void Post(WorkItem item)
  {
    // stop further work processing once the abort
    if (shutdown_)
    {
      return;
    }

    FETCH_LOCK(mutex_);
    store_.emplace_back(std::move(item));
  }

  IdleWorkStore operator=(const IdleWorkStore &rhs) = delete;
  IdleWorkStore operator=(IdleWorkStore &&rhs) = delete;

private:
  using Clock     = std::chrono::system_clock;
  using Timestamp = Clock::time_point;
  using Flag      = std::atomic<bool>;
  using Mutex     = fetch::mutex::Mutex;
  using Store     = std::vector<WorkItem>;

  mutable Mutex             mutex_{__LINE__, __FILE__};
  Store                     store_;
  Flag                      shutdown_{false};
  std::chrono::milliseconds interval_{0};
  mutable Timestamp         last_run_ = Clock::now();
};

}  // namespace details
}  // namespace network
}  // namespace fetch
