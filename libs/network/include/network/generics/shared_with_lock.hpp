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

#include <mutex>
#include <utility>

namespace fetch {
namespace generics {

template <class T>
class SharedWithLock
{
public:
  using mutex_type = std::mutex;
  using lock_type  = std::unique_lock<mutex_type>;

  SharedWithLock()
  {}

  SharedWithLock(const SharedWithLock &other)
    : mutex_p(other.mutex_p)
    , data_p(other.data_p)
  {}

  SharedWithLock &operator=(const SharedWithLock &other)
  {
    if (&other != this)
    {
      mutex_p = other.mutex_p;
      data_p  = other.data_p;
    }
    return *this;
  }

  operator bool() const
  {
    lock_type lock(*mutex_p);
    return bool(data_p);
  }

  virtual ~SharedWithLock()
  {}

  class Locked
  {
    friend class SharedWithLock;

  private:
    lock_type          lock_;
    std::shared_ptr<T> ptr_;

    Locked(std::shared_ptr<T> ptr, mutex_type &m)
      : lock_(m)
      , ptr_(std::move(ptr))
    {}

  public:
    Locked(Locked &&other)
      : lock_(std::move(other.lock_))
      , ptr_(other.ptr_)
    {}

    ~Locked()
    {}

  public:
    T *operator->()
    {
      return ptr_.get();
    }
    const T *operator->() const
    {
      return ptr_.get();
    }
    T &operator*()
    {
      return *ptr_;
    }
    const T &operator*() const
    {
      return *ptr_;
    }
  };

  template <typename... Args>
  void Make(Args &&... args)
  {
    mutex_p = std::make_shared<mutex_type>();
    data_p  = std::make_shared<T>(std::forward<Args>(args)...);
  }

  Locked Lock()
  {
    return Locked(data_p, *mutex_p);
  }

  void CopyOut(T &target)
  {
    lock_type lock(*mutex_p);
    target = *data_p;
  }

public:
  std::shared_ptr<mutex_type> mutex_p;
  std::shared_ptr<T>          data_p;
};

}  // namespace generics
}  // namespace fetch
