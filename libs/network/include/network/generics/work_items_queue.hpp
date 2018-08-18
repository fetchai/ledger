#ifndef WORK_ITEMS_QUEUE_HPP
#define WORK_ITEMS_QUEUE_HPP

#include <iostream>
#include <string>

namespace fetch {
namespace generics {

template <class TYPE>
class WorkItemsQueue
{
  using mutex_type = std::mutex;
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
    lock_type lock(mutex_);
    q.push_back(item);
  }

  template <class ITERATOR_GIVING_TYPE>
  void Add(ITERATOR_GIVING_TYPE iter, ITERATOR_GIVING_TYPE end)
  {
    lock_type lock(mutex_);
    while (iter != end)
    {
      q.push_back(*iter);
      ++iter;
    }
  }

  bool Empty(void)
  {
    lock_type lock(mutex_);
    return q.empty();
  }

  bool Remaining(void)
  {
    lock_type lock(mutex_);
    return !q.empty();
  }

  size_t Get(std::vector<TYPE> &output, size_t limit)
  {
    lock_type lock(mutex_);
    output.reserve(limit);
    while (!q.empty() && output.size() < limit)
    {
      output.push_back(q.front());
      q.pop_front();
    }
    return output.size();
  }

private:
  // members here.

  mutex_type mutex_;
  store_type q;
};

}  // namespace generics
}  // namespace fetch

#endif  // WORK_ITEMS_QUEUE_HPP
