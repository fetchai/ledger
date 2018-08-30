#ifndef WORK_ITEMS_QUEUE_HPP
#define WORK_ITEMS_QUEUE_HPP

#include <iostream>
#include <string>
#include <list>
#include <condition_variable>

namespace fetch {
namespace generics {

template <class TYPE>
class WorkItemsQueue
{
  using mutex_type = fetch::mutex::Mutex;
  using cv_type = std::condition_variable;
  using lock_type  = std::unique_lock<mutex_type>;
  using store_type = std::list<TYPE>;

public:
  WorkItemsQueue(const WorkItemsQueue &rhs) = delete;
  WorkItemsQueue(WorkItemsQueue &&rhs)      = delete;
  WorkItemsQueue &operator=(const WorkItemsQueue &rhs) = delete;
  WorkItemsQueue &operator=(WorkItemsQueue &&rhs)             = delete;
  bool            operator==(const WorkItemsQueue &rhs) const = delete;
  bool            operator<(const WorkItemsQueue &rhs) const  = delete;

  explicit WorkItemsQueue() {}

  virtual ~WorkItemsQueue() {}

  void Add(const TYPE &item)
  {
    {
      lock_type lock(mutex_);
      q.push_back(item);
      count_++;
    }
    cv_.notify_one();
  }

  template <class ITERATOR_GIVING_TYPE>
  void Add(ITERATOR_GIVING_TYPE iter, ITERATOR_GIVING_TYPE end)
  {
    {
      lock_type lock(mutex_);
    while(iter != end)
      {
        q.push_back(*iter);
        ++iter;
        count_++;
      }
    }
    cv_.notify_one();
  }

  bool empty(void) const
  {
    return count_.load() == 0;
  }

  size_t size(void) const
  {
    return count_.load();
  }

  bool Remaining(void)
  {
    lock_type lock(mutex_);
    return !q.empty();
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

  size_t Get(std::vector<TYPE> &output, size_t limit)
  {
    lock_type lock(mutex_);
    output.reserve(limit);
    while (!q.empty() && output.size() < limit)
    {
      output.push_back(q.front());
      q.pop_front();
      count_--;
    }
    return output.size();
  }

private:
  // members here.

  mutex_type mutex_{__LINE__, __FILE__};
  store_type q;
  cv_type cv_;
  std::atomic<bool> quit_{false};
  std::atomic<size_t> count_{0};
};

}  // namespace generics
}  // namespace fetch

#endif  // WORK_ITEMS_QUEUE_HPP
