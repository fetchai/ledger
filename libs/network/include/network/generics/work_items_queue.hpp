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

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <list>
#include <mutex>
#include <vector>

namespace fetch {
namespace generics {

template <class TYPE>
class WorkItemsQueue
{
  using mutex_type = std::mutex;
  using cv_type    = std::condition_variable;
  using lock_type  = std::unique_lock<mutex_type>;
  using store_type = std::list<TYPE>;

public:
  WorkItemsQueue(const WorkItemsQueue &rhs) = delete;
  WorkItemsQueue(WorkItemsQueue &&rhs)      = delete;
  WorkItemsQueue &operator=(const WorkItemsQueue &rhs) = delete;
  WorkItemsQueue &operator=(WorkItemsQueue &&rhs)             = delete;
  bool            operator==(const WorkItemsQueue &rhs) const = delete;
  bool            operator<(const WorkItemsQueue &rhs) const  = delete;

  WorkItemsQueue() = default;

  virtual ~WorkItemsQueue() = default;

  void Add(const TYPE &item)
  {
    {
      lock_type lock(mutex_);
      q_.push_back(item);
      count_++;
    }
    cv_.notify_one();
  }

  template <class ITERATOR_GIVING_TYPE>
  void Add(ITERATOR_GIVING_TYPE iter, ITERATOR_GIVING_TYPE end)
  {
    {
      lock_type lock(mutex_);
      while (iter != end)
      {
        q_.push_back(*iter);
        ++iter;
        count_++;
      }
    }
    cv_.notify_one();
  }

  bool empty() const
  {
    return count_.load() == 0;
  }

  std::size_t size() const
  {
    return count_.load();
  }

  bool Remaining()
  {
    lock_type lock(mutex_);
    return !q_.empty();
  }

  void Quit()
  {
    quit_.store(true);
    cv_.notify_all();
  }

  bool Wait()
  {
    if (size() > 0)
    {
      return true;
    }
    lock_type lock(mutex_);
    cv_.wait(lock);
    return !quit_.load();
  }

  std::size_t Get(std::vector<TYPE> &output, std::size_t limit)
  {
    lock_type lock(mutex_);
    output.reserve(limit);
    while (!q_.empty() && output.size() < limit)
    {
      output.push_back(q_.front());
      q_.pop_front();
      count_--;
    }
    return output.size();
  }

private:
  // members here.

  mutex_type               mutex_;
  store_type               q_;
  cv_type                  cv_;
  std::atomic<bool>        quit_{false};
  std::atomic<std::size_t> count_{0};
};

}  // namespace generics
}  // namespace fetch
