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

#include "oef-base/threading/Task.hpp"

#include <atomic>
#include <chrono>
#include <unordered_map>

class IdCache : public fetch::oef::base::Task
{
public:
  using Mutex = std::mutex;
  using Lock  = std::lock_guard<Mutex>;

  static constexpr char const *LOGGING_NAME = "IdCache";

  IdCache(uint64_t time_limit_sec, uint64_t cleaner_pool_period_sec)
    : cache_{}
    , time_limit_{std::move(time_limit_sec)}
    , cleaner_pool_period_(cleaner_pool_period_sec * 1000)
    , active_(true)
  {}
  ~IdCache() override           = default;
  IdCache(const IdCache &other) = delete;
  IdCache &operator=(const IdCache &other) = delete;

  bool operator==(const IdCache &other) = delete;
  bool operator<(const IdCache &other)  = delete;

  void Add(const uint64_t &id)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "ADD id to cache: ", id);
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    Lock lg(mutex_);
    cache_[id] = static_cast<uint64_t>(time);
  }

  bool IsCached(const uint64_t &id)
  {
    Lock lg(mutex_);
    bool result = false;
    auto it     = cache_.find(id);
    if (it != cache_.end())
    {
      auto     now  = std::chrono::system_clock::now();
      uint64_t time = static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
      result = (it->second + time_limit_) >= time;
    }
    return result;
  }

  void StopCacheCleaner()
  {
    active_.store(false);
  }

  bool IsRunnable() const override
  {
    return true;
  }

  fetch::oef::base::ExitState run() override
  {
    {
      Lock lg(mutex_);
      FETCH_LOG_INFO(LOGGING_NAME, "Run cleanup, cache size=", cache_.size());
      auto     it   = cache_.begin();
      auto     now  = std::chrono::system_clock::now();
      uint64_t time = static_cast<uint64_t>(
          std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count());
      while (it != cache_.end())
      {
        if ((it->second + time_limit_) < time)
        {
          it = cache_.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
    if (active_.load())
    {
      this->submit(cleaner_pool_period_);
    }
    return fetch::oef::base::ExitState ::COMPLETE;
  }

protected:
  Mutex                                  mutex_;
  std::unordered_map<uint64_t, uint64_t> cache_;
  uint64_t                               time_limit_;  // seconds
  std::chrono::milliseconds              cleaner_pool_period_;
  std::atomic<bool>                      active_;
};
