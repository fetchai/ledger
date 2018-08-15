#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch AI
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
protected:
  using work_func_type    = std::function<void()>;
  using due_date_type     = std::chrono::time_point<std::chrono::system_clock>;
  using work_item_type    = std::pair<due_date_type, work_func_type>;
  using heap_storage_type = std::vector<work_item_type>;
  using mutex_type        = std::recursive_mutex;
  using lock_type         = std::lock_guard<mutex_type>;

public:
  FutureWorkStore(const FutureWorkStore &rhs) = delete;
  FutureWorkStore(FutureWorkStore &&rhs)      = delete;
  FutureWorkStore operator=(const FutureWorkStore &rhs) = delete;
  FutureWorkStore operator=(FutureWorkStore &&rhs)             = delete;
  bool            operator==(const FutureWorkStore &rhs) const = delete;
  bool            operator<(const FutureWorkStore &rhs) const  = delete;

  class WorkItemSorting
  {
  public:
    WorkItemSorting() {}
    virtual ~WorkItemSorting() {}

    bool operator()(const work_item_type &a, const work_item_type &b) const
    {
      return a.first > b.first;
    }
  };

  FutureWorkStore() { std::make_heap(workStore_.begin(), workStore_.end(), sorter_); }

  virtual ~FutureWorkStore() {}

  virtual bool IsDue()
  {
    lock_type mlock(mutex_);
    if (workStore_.empty())
    {
      return false;
    }
    auto nextDue = workStore_.back();

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

  virtual work_func_type GetNext()
  {
    lock_type mlock(mutex_);
    std::pop_heap(workStore_.begin(), workStore_.end(), sorter_);
    auto nextDue = workStore_.back();
    workStore_.pop_back();
    return nextDue.second;
  }

  template <typename F>
  void Post(F &&f, uint32_t milliseconds)
  {
    lock_type mlock(mutex_);
    auto      dueTime = std::chrono::system_clock::now() + std::chrono::milliseconds(milliseconds);
    workStore_.push_back(work_item_type(dueTime, f));
    std::push_heap(workStore_.begin(), workStore_.end(), sorter_);
  }

private:
  WorkItemSorting    sorter_;
  heap_storage_type  workStore_;
  mutable mutex_type mutex_;
};

}  // namespace details
}  // namespace network
}  // namespace fetch
