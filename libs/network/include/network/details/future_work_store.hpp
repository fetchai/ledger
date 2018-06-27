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
  typedef std::function<void ()> WORK_FUNCTION;
  typedef std::chrono::time_point<std::chrono::system_clock> DUE_DATE;
  typedef std::pair<DUE_DATE, WORK_FUNCTION> WORK_ITEM;
  typedef std::vector<WORK_ITEM> HEAP_STORAGE;
  typedef std::recursive_mutex MUTEX_T;
  typedef std::lock_guard<MUTEX_T> LOCK_T;
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

    bool operator()(const WORK_ITEM &a, const WORK_ITEM &b) const
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

  bool isDue()
  {
    LOCK_T mlock(mutex_);
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

  WORK_FUNCTION getNext()
  {
    LOCK_T mlock(mutex_);
    std::pop_heap(workStore_.begin(), workStore_.end(), sorter_);
    auto nextDue = workStore_.back();
    workStore_.pop_back();
    return nextDue.second;
  }

  template <typename F>
  void Post(F &&f, int milliseconds)
  {
    LOCK_T mlock(mutex_);
    auto dueTime = std::chrono::system_clock::now() + std::chrono::milliseconds(5);
    workStore_.push_back(WORK_ITEM(dueTime, f));
    std::push_heap(workStore_.begin(), workStore_.end(), sorter_);
  }

private:
  WorkItemSorting sorter_;
  HEAP_STORAGE workStore_;
  mutable MUTEX_T mutex_;
};

}
}
}

#endif //__FUTURE_WORK_STORE__
