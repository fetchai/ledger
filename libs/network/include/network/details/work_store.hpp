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

#include "core/mutex.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <deque>

namespace fetch {
namespace network {
namespace details {

class WorkStore
{
public:
  using WorkItem = std::function<void()>;

  WorkStore() = default;
  WorkStore(const WorkStore &rhs) = delete;
  WorkStore(WorkStore &&rhs)      = delete;
  ~WorkStore()
  {
    shutdown_.store(true);
    clear();
  }

  WorkStore operator=(const WorkStore &rhs) = delete;
  WorkStore operator=(WorkStore &&rhs)      = delete;

  void clear()
  {
    FETCH_LOCK(mutex_);
    store_.clear();
  }

  bool empty()
  {
    FETCH_LOCK(mutex_);
    return store_.empty();
  }

  void Abort()
  {
    shutdown_.store(true);
  }

  bool GetNext(WorkItem &output)
  {
    FETCH_LOCK(mutex_);
    if (shutdown_.load())
    {
      return false;
    }
    if (store_.empty())
    {
      return false;
    }
    output = GetNextActual();
    return true;
  }

  int Visit(std::function<void (WorkItem)> const &visitor, int maxprocess=1)
  {
    int processed = 0;
    while(true)
    {
      WorkItem work;
      if (processed >= maxprocess)
      {
        break;
      }
      if (!GetNext(work))
      {
        break;
      }
      visitor(work);
      processed++;
    }
    return processed;
  }

  template <typename F>
  void Post(F &&f)
  {
    if (shutdown_.load())
    {
      return;
    }
    FETCH_LOCK(mutex_);
    store_.push_back(f);
  }

private:

  using Queue    = std::deque<WorkItem>;
  using Mutex    = mutex::Mutex;

  Queue store_;
  mutable Mutex mutex_{__LINE__, __FILE__};
  std::atomic<bool> shutdown_{false};

  WorkItem GetNextActual()
  {
    auto next_due = store_.front();
    store_.pop_front();
    return next_due;
  }
};

}  // namespace details
}  // namespace network
}  // namespace fetch
