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
#include <deque>
#include <functional>
#include <string>

namespace fetch {
namespace network {
namespace details {

/**
 * Simple FIFO based work item queue
 */
class WorkStore
{
public:
  using WorkItem = std::function<void()>;

  WorkStore()                     = default;
  WorkStore(const WorkStore &rhs) = delete;
  WorkStore(WorkStore &&rhs)      = delete;

  ~WorkStore()
  {
    shutdown_ = true;
    Clear();
  }

  /**
   * Determine if the queue is empty
   * @return true if the queue is empty, otherwise false
   */
  bool IsEmpty() const
  {
    FETCH_LOCK(mutex_);
    return queue_.empty();
  }

  /**
   * Clear all queued items from the work queue
   */
  void Clear()
  {
    FETCH_LOCK(mutex_);
    queue_.clear();
  }

  /**
   * Signal that the work queue should accept no further work items
   */
  void Abort()
  {
    shutdown_ = true;
  }

  /**
   * Extract and dispatch a single item from the queue
   *
   * @tparam CALLBACK The type of the callable accepting the signature: void(WorkItem const &)
   * @param handler The dispatching function
   * @return The number of items processed
   */
  template <typename CALLBACK>
  std::size_t Dispatch(CALLBACK const &handler)
  {
    WorkItem    work;
    std::size_t num_processed = 0;

    // attempt to extract a piece of work from the queue
    {
      FETCH_LOCK(mutex_);
      if (!queue_.empty())
      {
        work = queue_.front();
        queue_.pop_front();
      }
    }

    // exit the dispatch loop in the case when no work has been found
    if (work)
    {
      // execute the callback handler on the piece of work
      handler(work);

      ++num_processed;
    }

    return num_processed;
  }

  /**
   * Add a work item to back of the queue
   *
   * @param work The work item to be executed
   */
  void Post(WorkItem work)
  {
    // prevent further posts after shutdown
    if (shutdown_)
    {
      return;
    }

    FETCH_LOCK(mutex_);
    queue_.emplace_back(std::move(work));
  }

  WorkStore operator=(const WorkStore &rhs) = delete;
  WorkStore operator=(WorkStore &&rhs) = delete;

private:
  using Queue = std::deque<WorkItem>;
  using Mutex = mutex::Mutex;

  mutable Mutex     mutex_{__LINE__, __FILE__};  ///< Mutex protecting `queue_`
  Queue             queue_;                      ///< The queue of work items
  std::atomic<bool> shutdown_{false};            ///< Flag to signal the work queue is shutting down
};

}  // namespace details
}  // namespace network
}  // namespace fetch
