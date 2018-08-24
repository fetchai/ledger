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

#include <algorithm>
#include <iostream>
#include <string>

namespace fetch {
namespace network {
namespace details {

class WorkStore
{
public:
  using work_item_type    = std::function<void()>;
protected:
  using storage_type      = std::queue<work_item_type>;
  using mutex_type        = std::mutex;
  using lock_type         = std::lock_guard<mutex_type>;

public:
  WorkStore(const WorkStore &rhs) = delete;
  WorkStore(WorkStore &&rhs)      = delete;
  WorkStore operator=(const WorkStore &rhs) = delete;
  WorkStore operator=(WorkStore &&rhs)             = delete;
  bool            operator==(const WorkStore &rhs) const = delete;
  bool            operator<(const WorkStore &rhs) const  = delete;

  WorkStore() { }

  virtual ~WorkStore()
  {
    shutdown_.store(true);
    lock_type mlock(mutex_); // wait in case anyone is in here.
    while(!store_.empty())
    {
      store_.pop();
    }
  }

  virtual void clear()
  {
    lock_type mlock(mutex_);
    while(!store_.empty())
    {
      store_.pop();
    }
  }

  virtual bool empty()
  {
    lock_type mlock(mutex_);
    return store_.empty();
  }

  virtual void Abort()
  {
    shutdown_.store(true);
  }

  virtual work_item_type GetNext()
  {
    lock_type mlock(mutex_);
    return GetNextActual();
  }

  virtual int Visit(std::function<void (work_item_type)> visitor, int maxprocess=1)
  {
    int processed = 0;
    while(true)
    {
      if (shutdown_.load()) break;
      if (processed >= maxprocess) break;
      if (empty()) break;
      auto work = GetNext();
      visitor(work);
      processed++;
    }
    return processed;
  }

  template <typename F>
  void Post(F &&f)
  {
    if (shutdown_.load()) return;
    lock_type mlock(mutex_);
    store_.push(f);
  }

private:
  storage_type store_;
  mutable mutex_type mutex_;
  std::atomic<bool> shutdown_{false};

  virtual work_item_type GetNextActual()
  {
    auto nextDue = store_.front();
    store_.pop();
    return nextDue;
  }
};

}  // namespace details
}  // namespace network
}  // namespace fetch
