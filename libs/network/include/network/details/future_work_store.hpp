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
public:
  FutureWorkStore(const FutureWorkStore &rhs)           = delete;
  FutureWorkStore(FutureWorkStore &&rhs)           = delete;
  FutureWorkStore operator=(const FutureWorkStore &rhs)  = delete;
  FutureWorkStore operator=(FutureWorkStore &&rhs) = delete;
  bool operator==(const FutureWorkStore &rhs) const = delete;
  bool operator<(const FutureWorkStore &rhs) const = delete;

  typedef std::function<void ()> WORK_FUNCTION;
  typedef std::chrono::time_point<std::chrono::system_clock> DUE_DATE;
  typedef std::pair<DUE_DATE, WORK_FUNCTION> WORK_ITEM;

  //typedef int WORK_FUNCTION;
  //typedef long DUE_DATE;
  //typedef int WORK_ITEM;

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

  WorkItemSorting sorter;
  
  typedef std::vector<WORK_ITEM> HEAP_STORAGE;
  HEAP_STORAGE workStore;

  typedef std::recursive_mutex MUTEX_T;
  typedef std::lock_guard<MUTEX_T> LOCK_T;
  mutable MUTEX_T mutex_;
  
  FutureWorkStore()
  {
    std::make_heap(workStore.begin(), workStore.end(), sorter);
  }
  
  virtual ~FutureWorkStore()
  {
  }

  bool isDue(std::size_t id) 
  {
    LOCK_T mlock(mutex_);
    if (workStore.empty())
      {
	return false;
      }
    auto nextDue = workStore.back();

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

  WORK_FUNCTION getNext(std::size_t id)
  {
    LOCK_T mlock(mutex_);
    std::pop_heap(workStore.begin(), workStore.end(), sorter);
    auto nextDue = workStore.back();
    workStore.pop_back();
    return nextDue.second;
  }

  template <typename F>
  void Post(F &&f, int milliseconds)
  {
    LOCK_T mlock(mutex_);
    auto dueTime = std::chrono::system_clock::now() + std::chrono::milliseconds(5);
    workStore.push_back(WORK_ITEM(dueTime, f));
    std::push_heap(workStore.begin(), workStore.end(), sorter);
  }
};

}
}
}

#endif //__FUTURE_WORK_STORE__
