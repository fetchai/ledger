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

class IdleWorkStore
{
public:
  using work_item_type    = std::function<void()>;
protected:
  using store_type        = std::vector<work_item_type>;
  using mutex_type        = fetch::mutex::Mutex;
  using lock_type         = std::unique_lock<mutex_type>;

public:
  IdleWorkStore(const IdleWorkStore &rhs) = delete;
  IdleWorkStore(IdleWorkStore &&rhs)      = delete;
  IdleWorkStore operator=(const IdleWorkStore &rhs) = delete;
  IdleWorkStore operator=(IdleWorkStore &&rhs)             = delete;
  bool            operator==(const IdleWorkStore &rhs) const = delete;
  bool            operator<(const IdleWorkStore &rhs) const  = delete;

  IdleWorkStore() { }

  virtual ~IdleWorkStore()
  {
    shutdown_.store(true);
    lock_type mlock(mutex_); // wait in case anyone is in here.
    store_.clear(); // remove any pending things
  }

  virtual void clear()
  {
    lock_type mlock(mutex_);
    store_.clear();
  }

  virtual void Abort()
  {
    shutdown_.store(true);
  }

  virtual int Visit(std::function<void (work_item_type)> visitor)
  {
    lock_type mlock(mutex_, std::try_to_lock);
    if (!mlock)
    {
      return -1;
    }
    int processed = 0;
    for(auto &work : store_)
    {
      if (shutdown_.load()) break;
      visitor(work);
      processed++;
    }
    return processed;
  }

  template <typename F>
  void Post(F &&f)
  {
    if (shutdown_.load()) return;
    lock_type mlock(mutex_);
    store_.push_back(f);
  }

private:
  store_type      store_;
  mutable mutex_type mutex_{__LINE__, __FILE__};
  std::atomic<bool> shutdown_{false};
};

}  // namespace details
}  // namespace network
}  // namespace fetch
