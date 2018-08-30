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

class FutureWorkStore
{
public:
  using work_item_type    = std::function<void()>;
protected:
  using due_date_type     = std::chrono::time_point<std::chrono::system_clock>;
  using stored_work_item_type    = std::pair<due_date_type, work_item_type>;
  using store_type        = std::vector<stored_work_item_type>;
  using mutex_type          = fetch::mutex::Mutex;
  using lock_type           = std::unique_lock<mutex_type>;

public:
  FutureWorkStore(const FutureWorkStore &rhs) = delete;
  FutureWorkStore(FutureWorkStore &&rhs)      = delete;
  FutureWorkStore operator=(const FutureWorkStore &rhs) = delete;
  FutureWorkStore operator=(FutureWorkStore &&rhs)             = delete;
  bool            operator==(const FutureWorkStore &rhs) const = delete;
  bool            operator<(const FutureWorkStore &rhs) const  = delete;

  class StoredWorkItemSorting
  {
  public:
    StoredWorkItemSorting() {}
    virtual ~StoredWorkItemSorting() {}

    bool operator()(const stored_work_item_type &a, const stored_work_item_type &b) const
    {
      return a.first > b.first;
    }
  };

  FutureWorkStore() { std::make_heap(store_.begin(), store_.end(), sorter_); }

  virtual ~FutureWorkStore()
  {
    shutdown_.store(true);
    lock_type mlock(mutex_); // wait in case anyone is in here.
    store_.clear(); // remove any pending things
  }

  virtual void Abort()
  {
    shutdown_.store(true);
  }

  virtual void clear()
  {
    lock_type mlock(mutex_);
    store_.clear();
  }

  virtual bool IsDue()
  {
    lock_type mlock(mutex_);
    return IsDueActual();
  }

  virtual int Visit(std::function<void (work_item_type)> visitor, int maxprocesses=1)
  {
    lock_type mlock(mutex_, std::try_to_lock);
    if (!mlock)
    {
      return -1;
    }
    int processed = 0;
    while(IsDueActual())
    {
      if (shutdown_.load()) break;
      if (processed >= maxprocesses) break;
      auto work = GetNextActual();
      visitor(work);
      processed++;
    }
    return processed;
  }

  virtual work_item_type GetNext()
  {
    lock_type mlock(mutex_);
    return GetNextActual();
  }

  template <typename F>
  void Post(F &&f, uint32_t milliseconds)
  {
    if (shutdown_.load()) return;
    lock_type mlock(mutex_);
    auto      dueTime = std::chrono::system_clock::now() + std::chrono::milliseconds(milliseconds);
    store_.push_back(stored_work_item_type(dueTime, f));
    std::push_heap(store_.begin(), store_.end(), sorter_);
  }

private:

  virtual work_item_type GetNextActual()
  {
    std::pop_heap(store_.begin(), store_.end(), sorter_);
    auto nextDue = store_.back();
    store_.pop_back();
    return nextDue.second;
  }

  bool IsDueActual()
  {
    if (shutdown_.load())
    {
      return false;
    }
    if (store_.empty())
    {
      return false;
    }
    auto nextDue = store_.back();

    auto tp     = std::chrono::system_clock::now();
    auto due    = nextDue.first;
    auto wayoff = (due - tp).count();

    if (wayoff < 0)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  StoredWorkItemSorting    sorter_;
  store_type         store_;
  mutable mutex_type mutex_{__LINE__, __FILE__};
  std::atomic<bool> shutdown_{false};
};

}  // namespace details
}  // namespace network
}  // namespace fetch
