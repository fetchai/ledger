#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include <deque>
#include <memory>
#include <mutex>
#include <typeindex>

namespace fetch {
namespace beacon {

class EventManager
{
public:
  using SharedEventManager = std::shared_ptr<EventManager>;

  static SharedEventManager New()
  {
    SharedEventManager ret;
    ret.reset(new EventManager());
    return ret;
  }

  template <typename T>
  void Dispatch(T const &event)
  {
    FETCH_LOCK(mutex_);
    std::type_index index{typeid(T)};

    auto it = events_.find(index);
    if (it == events_.end())
    {
      events_[index] = std::deque<void *>();
    }

    auto *ptr = reinterpret_cast<void *>(new T(event));
    events_[index].push_back(ptr);
  }

  template <typename T>
  bool Poll(T &event)
  {
    FETCH_LOCK(mutex_);
    std::type_index index{typeid(T)};

    auto it = events_.find(index);
    if (it == events_.end())
    {
      return false;
    }

    if (it->second.empty())
    {
      return false;
    }

    auto *ptr = reinterpret_cast<T *>(it->second.front());
    it->second.pop_front();

    event = *ptr;
    delete ptr;

    return true;
  }

private:
  EventManager() = default;

  std::unordered_map<std::type_index, std::deque<void *>> events_;
  Mutex                                                   mutex_;
};

}  // namespace beacon
}  // namespace fetch
