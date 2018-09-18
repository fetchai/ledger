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
#include "core/serializers/stl_types.hpp"

namespace fetch {
namespace network {

template<class KEY, class REQUESTED, class PROMISE = PromiseOf<REQUESTED>>
class RequestingQueueOf
{
public:
  using Mutex = fetch::mutex::Mutex;
  using Lock = std::lock_guard<Mutex>;
  using Promised = PROMISE;
  using State = typename Promised::State;
  using PendingPromised = std::map<KEY, Promised>;
  using OutputType = std::pair<KEY, REQUESTED>;
  using FailedOutputType = std::pair<KEY, PROMISE>;
  using Timepoint = typename Promised::Timepoint;

  static constexpr char const *LOGGING_NAME = "RequestingQueueOf";

  RequestingQueueOf()
  {
    count_ . store(0);
    failcount_ . store(0);
    pending_count_ . store(0);
  }

  virtual ~RequestingQueueOf()
  {
  }

  std::tuple<size_t, size_t, size_t>  Resolve(const Timepoint &tp)
  {
    FETCH_LOG_WARN(LOGGING_NAME,"Resolve(Timepoint) start DONE=", count_.load(), "  FAIL=", failcount_.load(),  "   WAIT=", pending_count_.load());
    Lock lock(mutex_);
    {
      auto iter = requests_ . begin();
      while(iter != requests_ . end())
      {
        auto const &key = (*iter) . first;
        auto &prom = (*iter) . second;
        switch(prom . GetState(tp))
        {
        case State::WAITING:
          ++iter;
          break;
        case State::SUCCESS:
          {
            REQUESTED result = prom . Get();
            completed_ . push_back(std::make_pair(key, result));
            count_++;
            pending_count_--;
          }
          pending_count_--;
          iter = requests_ . erase(iter);
          break;
        case State::TIMEDOUT:
        case State::FAILED:
          failed_ . push_back(std::make_pair(key, prom));
          failcount_++;
          iter = requests_ . erase(iter);
          pending_count_--;
          break;
        }
      }
    }

    count_.store( completed_.size());
    failcount_.store( failed_.size());
    pending_count_.store( requests_.size());
    FETCH_LOG_WARN(LOGGING_NAME,"Resolve(Timepoint) ended DONE=", count_.load(), "  FAIL=", failcount_.load(),  "   WAIT=", pending_count_.load());
    return std::make_tuple(  count_.load(), failcount_.load(),  pending_count_.load());
  }

  std::tuple<size_t, size_t, size_t> Resolve()
  {
    FETCH_LOG_WARN(LOGGING_NAME,"Resolve");
    Lock lock(mutex_);
    {
      auto iter = requests_ . begin();
      while(iter != requests_ . end())
      {
        auto const &key = (*iter) . first;
        auto &prom = (*iter) . second;
        switch(prom . GetState())
        {
        case State::WAITING:
          ++iter;
          break;
        case State::SUCCESS:
          {
            REQUESTED result = prom . Get();
            completed_ . push_back(std::make_pair(key, result));
          }
          count_++;
          iter = requests_ . erase(iter);
          pending_count_--;
          break;
        case State::TIMEDOUT:
        case State::FAILED:
          failed_ . push_back(std::make_pair(key, prom));
          failcount_++;
          iter = requests_ . erase(iter);
          pending_count_--;
          break;
        }
      }
    }

    count_.store( completed_.size());
    failcount_.store( failed_.size());
    pending_count_.store( requests_.size());
    FETCH_LOG_INFO("RequestingQueueOf","Resolve leaves ", pending_count_.load());

    return std::make_tuple(  count_.load(), failcount_.load(),  pending_count_.load());
  }

  void Add(const KEY &key, const PROMISE &new_promise)
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
      if (requests_ . find(key) == requests_ . end())
      {
        result.push_back(key);
      }
    }
    return result;
  }

  bool Inflight(const KEY &key) const
  {
    Lock lock(mutex_);
    return (requests_ . find(key) != requests_ . end());
  }

  size_t Get(std::vector<OutputType> &output, size_t limit)
  {
    Lock lock(mutex_);
    output.reserve(limit);
    output.clear();
    while (!completed_.empty() && output.size() < limit)
    {
      output.push_back(completed_.front());
      completed_.pop_front();
      count_--;
    }
    return output.size();
  }

  size_t GetFailures(std::vector<FailedOutputType> &output, size_t limit)
  {
    Lock lock(mutex_);
    output.reserve(limit);
    output.clear();
    while (!failed_.empty() && output.size() < limit)
    {
      output.push_back(failed_.front());
      failed_.pop_front();
      failcount_--;
    }
    return output.size();
  }

  size_t failcount() { return failcount_ . load(); }

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
    return failcount_.load() > 0;
  }

  bool RemainingFailures(void)
  {
    return count_.load() > 0;
  }

  void DiscardFailures()
  {
    failed_ . clear();
  }
private:
  PendingPromised requests_;
  mutable Mutex mutex_{__LINE__, __FILE__};
  std::atomic<size_t> count_;
  std::atomic<size_t> failcount_;
  std::atomic<size_t> pending_count_;

  std::list<OutputType> completed_;

  std::list<FailedOutputType> failed_;
};

}  // namespace network
}  // namespace fetch
