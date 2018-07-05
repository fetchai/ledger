#ifndef __FUTURE_WORK_STORE__
#define __FUTURE_WORK_STORE__

#include <algorithm>
#include <iostream>
#include <string>
using std::cout;
using std::cerr;
using std::endl;
using std::string;

namespace fetch {
namespace network {
namespace details {

class FutureWorkStore
{
protected:
  typedef std::function<void ()> work_func_type;
  typedef std::chrono::time_point<std::chrono::system_clock> due_date_type;
  typedef std::pair<due_date_type, work_func_type> work_item_type;
  typedef std::vector<work_item_type> heap_storage_type;
  typedef std::recursive_mutex mutex_type;
  typedef std::lock_guard<mutex_type> lock_type;
public:
  FutureWorkStore(const FutureWorkStore &rhs)            = delete;
  FutureWorkStore(FutureWorkStore &&rhs)                 = delete;
  FutureWorkStore operator=(const FutureWorkStore &rhs)  = delete;
  FutureWorkStore operator=(FutureWorkStore &&rhs)       = delete;
  bool operator==(const FutureWorkStore &rhs) const      = delete;
  bool operator<(const FutureWorkStore &rhs) const       = delete;

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

  FutureWorkStore()
  {
    std::make_heap(workStore_.begin(), workStore_.end(), sorter_);
  }

  virtual ~FutureWorkStore()
  {
  }

  virtual bool IsDue()
  {
    lock_type mlock(mutex_);
    if (workStore_.empty())
      {
        return false;
      }
    auto nextDue = workStore_.back();

    auto tp = std::chrono::system_clock::now();
    auto due = nextDue.first;
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
  void Post(F &&f, int milliseconds)
  {
    lock_type mlock(mutex_);
    auto dueTime = std::chrono::system_clock::now() + std::chrono::milliseconds(milliseconds);
    workStore_.push_back(work_item_type(dueTime, f));
    std::push_heap(workStore_.begin(), workStore_.end(), sorter_);
  }

private:
  WorkItemSorting sorter_;
  heap_storage_type workStore_;
  mutable mutex_type mutex_;
};

}
}
}

#endif //__FUTURE_WORK_STORE__
