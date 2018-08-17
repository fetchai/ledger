#pragma once

#include <iostream>
#include <string>
#include <set>

#include "network/generics/locked.hpp"

namespace fetch
{
namespace generics
{

template<class TYPE>
class ThreadsafeSet
{
  using mutex_type = std::mutex;
  using lock_type = std::unique_lock<mutex_type>;
  using store_type = std::set<TYPE>;
public:
  ThreadsafeSet(const ThreadsafeSet &rhs)            = delete;
  ThreadsafeSet &operator=(const ThreadsafeSet &rhs) = delete;
  ThreadsafeSet &operator=(ThreadsafeSet &&rhs)      = delete;
  bool operator==(const ThreadsafeSet &rhs) const = delete;
  bool operator<(const ThreadsafeSet &rhs) const  = delete;

  ThreadsafeSet(ThreadsafeSet &&rhs)
    : store(std::move(rhs.store))
  {
  }

  explicit ThreadsafeSet()
  {
  }

  virtual ~ThreadsafeSet()
  {
  }

  bool empty() const
  {
    return sz_.load() == 0;
  }

  bool Has(const TYPE &item) const
  {
    lock_type lock(mutex_);
    return store.find(item) != store.end();
  }

  size_t size(void) const
  {
    return sz_.load();
  }

  bool Add(const TYPE &item)
  {
    lock_type lock(mutex_);
    auto r = store.insert(item).second;
    if (r)
    {
      sz_++;
    }
    return r;
  }
  bool Del(const TYPE &item)
  {
    lock_type lock(mutex_);
    auto r = store.remove(item);
    if (r)
    {
      sz_--;
    }
    return r;
  }

  Locked<store_type, mutex_type> GetLocked()
  {
    return Locked<store_type, mutex_type>(mutex_, store);
  }

  void VisitRemove(std::function<void (TYPE)> visitor)
  {
    lock_type lock(mutex_);

    for(auto &member : store)
    {
      visitor(member);
    }
    store.clear();
    sz_.store(0);
  }
private:
// members here.

  mutex_type mutex_;
  store_type store;
  std::atomic<size_t> sz_{0};
};

}
}
