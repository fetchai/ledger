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

#include "network/service/promise.hpp"
#include "network/generics/promise_of.hpp"

namespace fetch {
namespace network {

template<class KEY, class REQUESTED, class REQUESTED_CONTAINER = std::list<REQUESTED>>
class RequestingQueueOf
{
public:
  using Mutex = fetch::mutex::Mutex;
  using Lock = std::lock_guard<Mutex>;

  using ManyPromised = PromiseOf<REQUESTED_CONTAINER>;
  using Promised = PromiseOf<REQUESTED>;
  using State = typename Promised::State;

  using ManyPendingPromised = std::map<KEY, ManyPromised>;
  using PendingPromised = std::map<KEY, Promised>;

  using OutputType = std::pair<KEY, REQUESTED>;

  RequestingQueueOf()
  {
  }

  virtual ~RequestingQueueOf()
  {
  }

  void Resolve()
  {
    Lock lock(mutex_);

    {
      auto iter = many_requests_ . begin();
      while(iter != many_requests_ . end())
      {
        auto key = (*iter) . first;
        auto prom = (*iter) . second;
        switch(prom . GetState())
        {
        case State::WAITING:
          ++iter;
          pending_count_--;
          break;
        case State::SUCCESS:
          {
            REQUESTED_CONTAINER result = prom . Get();
            for(auto &item : result)
            {
              completed_ . push_back(std::make_pair(key, item));
              count_++;
            }
          }
          iter = many_requests_ . erase(iter);
          break;
        case State::FAILED:
          iter = many_requests_ . erase(iter);
          break;
        }
      }
    }

    {
      auto iter = requests_ . begin();
      while(iter != requests_ . end())
      {
        auto key = (*iter) . first;
        auto prom = (*iter) . second;
        switch(prom . GetState())
        {
        case State::WAITING:
          ++iter;
          pending_count_--;
          break;
        case State::SUCCESS:
          {
            REQUESTED result = prom . Get();
            completed_ . push_back(std::make_pair(key, result));
            count_++;
          }
          pending_count_--;
          iter = requests_ . erase(iter);
          break;
        case State::FAILED:
          iter = requests_ . erase(iter);
          pending_count_--;
          break;
        }
      }
    }

    count_.store( completed_.size());
    pending_count_.store( many_requests_.size() + requests_.size());
  }

  template<class SOME_CONTAINER>
  void Add(const KEY &key, const PromiseOf<SOME_CONTAINER> &new_promise)
  {
    Lock lock(mutex_);
    many_requests_ . insert (std::make_pair(key, new_promise));
    pending_count_++;
  }

  void Add(const KEY &key, const PromiseOf<REQUESTED> &new_promise)
  {
    Lock lock(mutex_);
    requests_ . insert (std::make_pair(key, new_promise));
    pending_count_++;
  }

  template<class CONTAINER_OF_KEYS>
  std::vector<KEY> FilterOutInflight(const CONTAINER_OF_KEYS &inputs)
  {
    Lock lock(mutex_);
    std::vector<KEY> result;
    for(auto &key : inputs)
    {
      if (
          (many_requests_ . find(key) == many_requests_ . end())
          &&
          (requests_ . find(key) == requests_ . end())
          )
      {
        result.push_back(key);
      }
    }
    return result;
  }

  bool Inflight(const KEY &key)
  {
    Lock lock(mutex_);
    return (
            (many_requests_ . find(key) != many_requests_ . end())
            ||
            (requests_ . find(key) != requests_ . end())
            );
  }

  size_t Get(std::vector<OutputType> &output, size_t limit)
  {
    Lock lock(mutex_);
    output.reserve(limit);
    while (!completed_.empty() && output.size() < limit)
    {
      output.push_back(completed_.front());
      completed_.pop_front();
      count_--;
    }
    return output.size();
  }

  size_t size(void) const
  {
    return count_.load();
  }

  bool empty(void) const
  {
    return count_.load() == 0;
  }

  bool Remaining(void)
  {
    return count_.load() > 0;
  }
private:
  ManyPendingPromised many_requests_;
  PendingPromised requests_;
  Mutex mutex_{__LINE__, __FILE__};
  std::atomic<size_t> count_;
  std::atomic<size_t> pending_count_;

  std::list<OutputType> completed_;

  std::list<KEY> failed_;
};

}  // namespace network
}  // namespace fetch
