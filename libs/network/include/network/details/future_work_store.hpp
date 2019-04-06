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
#include <queue>
#include <string>

namespace fetch {
namespace network {
namespace details {

/**
 * The future work store is a simple priority queue of work items. These work items are
 * stored alongside a due timestamp. The priority / order of the queue is determined by
 * these timestamps
 */
class FutureWorkStore
{
public:
  static constexpr char const *LOGGING_NAME = "FutureWorkStore";

  using WorkItem = std::function<void()>;

  // Construction / Destruction
  FutureWorkStore()                           = default;
  FutureWorkStore(const FutureWorkStore &rhs) = delete;
  FutureWorkStore(FutureWorkStore &&rhs)      = delete;
  ~FutureWorkStore()
  {
    shutdown_ = true;
    Clear();  // remove any pending things
  }

  /**
   * Signal that the work queue should no longer accept and work items
   */
  void Abort()
  {
    shutdown_ = true;
  }

  /**
   * Empty the queue of work items
   */
  void Clear()
  {
    FETCH_LOCK(queue_mutex_);
    while (!queue_.empty())
    {
      queue_.pop();
    }
  }

  /**
   * Extract and dispatch a single item from the queue
   *
   * @tparam CALLBACK The type of the callable accepting the signature: void(WorkItem const &)
   * @param visitor The dispatching function
   * @return The number of items processed
   */
  template <typename CALLBACK>
  std::size_t Dispatch(CALLBACK const &visitor)
  {
    std::size_t num_processed = 0;

    Timestamp const now = Clock::now();

    // allow early exit
    std::unique_lock<Mutex> lock(queue_mutex_, std::try_to_lock);
    if (lock.owns_lock())
    {
      WorkItem item;

      // empty loop condition
      if (!queue_.empty())
      {
        Element const &next = queue_.top();
        if (next.due <= now)
        {
          item = next.item;
          queue_.pop();
        }
      }

      // release the queue
      lock.unlock();

      // dispatch all the work items
      if (item)
      {
        // dispatch the work item
        visitor(item);

        // increment the counter
        ++num_processed;
      }
    }

    return num_processed;
  }

  /**
   * Add a work item with a specified delay in milliseconds
   *
   * @param item The work item to be added to the queue
   * @param milliseconds The delay in milliseconds before this work item is scheduled
   */
  void Post(WorkItem item, uint32_t milliseconds)
  {
    // reject further work if we are in the process of shutting down
    if (shutdown_)
    {
      return;
    }

    // add it to the queue
    {
      FETCH_LOCK(queue_mutex_);
      queue_.emplace(std::move(item), milliseconds);
    }
  }

  /**
   * Determine the time until the next available item in the queue
   *
   * @return milliseconds::max() if the work queue is empty, milliseconds::zero()
   * if the time is already due for execution, otherwise the time in milliseconds remaining
   */
  std::chrono::milliseconds TimeUntilNextItem()
  {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    Timestamp const now = Clock::now();

    {
      FETCH_LOCK(queue_mutex_);
      if (!queue_.empty())
      {
        Element const &element = queue_.top();

        if (element.due > now)
        {
          return duration_cast<milliseconds>(element.due - now);
        }
        else
        {
          return milliseconds::zero();
        }
      }
    }

    return milliseconds::max();
  }

  // Operators
  FutureWorkStore operator=(const FutureWorkStore &rhs) = delete;
  FutureWorkStore operator=(FutureWorkStore &&rhs) = delete;

private:
  using Clock     = std::chrono::system_clock;
  using Timestamp = Clock::time_point;

  struct Element
  {
    WorkItem  item;
    Timestamp due;

    Element(WorkItem i, uint32_t delay_ms)
      : item(std::move(i))
      , due{Clock::now() + std::chrono::milliseconds(delay_ms)}
    {}

    bool operator<(Element const &other) const
    {
      return due < other.due;
    }
  };

  using Queue = std::priority_queue<Element>;
  using Mutex = fetch::mutex::Mutex;
  using Flag  = std::atomic<bool>;

  mutable Mutex queue_mutex_{__LINE__, __FILE__};  ///< Mutex protecting `queue_`
  Queue         queue_;                            ///< Ordered queue of work items

  // Shutdown flag this is designed to only ever be set to true. User will have to recreate the
  // whole thread pool with current implementation.
  Flag shutdown_{false};
};

}  // namespace details
}  // namespace network
}  // namespace fetch
