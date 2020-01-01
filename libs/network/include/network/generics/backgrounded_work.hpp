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

#include "network/service/promise.hpp"

namespace fetch {
namespace network {

/**
 * This represents a number of promise-like tasks which can be polled
 * to see if they have finished and if so, if they succeeded, timed out
 * or failed. This could be done in a bg thread or by a foreground
 * polling process.
 *
 * @tparam WORKER The class type of our promise-like tasks.
 */
template <class WORKER>
class BackgroundedWork
{
public:
  using Worker       = std::shared_ptr<WORKER>;
  using PromiseState = fetch::service::PromiseState;
  using WorkLoad     = std::map<PromiseState, std::vector<Worker>>;
  using Results      = std::vector<Worker>;

  static constexpr char const *LOGGING_NAME = "BackgroundedWork";

  BackgroundedWork()
  {
    FETCH_LOCK(mutex_);
    workload_[PromiseState::WAITING];
    workload_[PromiseState::SUCCESS];
    workload_[PromiseState::FAILED];
    workload_[PromiseState::TIMEDOUT];
  }

  ~BackgroundedWork()
  {
    FETCH_LOCK(mutex_);
  }

  bool WorkCycle()
  {
    FETCH_LOCK(mutex_);

    if (workload_[PromiseState::WAITING].empty())
    {
      return false;
    }

    auto &worklist_for_state = workload_[PromiseState::WAITING];
    auto  workitem_iter      = worklist_for_state.begin();
    while (workitem_iter != worklist_for_state.end())
    {
      auto workitem = *workitem_iter;
      if (!workitem)
      {
        workitem_iter = worklist_for_state.erase(workitem_iter);
      }
      else
      {
        PromiseState result;
        try
        {
          result = workitem->Work();
        }
        catch (std::exception const &ex)
        {
          FETCH_LOG_WARN(LOGGING_NAME, "WorkCycle threw:", ex.what());
          result = PromiseState::FAILED;
        }
        switch (result)
        {
        case PromiseState::WAITING:
          ++workitem_iter;
          break;
        case PromiseState::SUCCESS:
        case PromiseState::FAILED:
        case PromiseState::TIMEDOUT:
          assert(std::size_t(result) < workload_.size());
          workload_[result].push_back(workitem);
          workitem_iter = worklist_for_state.erase(workitem_iter);
          break;
        }
      }
    }
    return true;
  }

  void Wait(int milliseconds)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, std::chrono::milliseconds(milliseconds));
  }

  template <typename Rep, typename Per>
  void Wait(std::chrono::duration<Rep, Per> const &timeout)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait_for(lock, timeout);
  }

  void Wake()
  {
    FETCH_LOCK(mutex_);
    cv_.notify_one();
  }

  void WakeAll()
  {
    FETCH_LOCK(mutex_);
    cv_.notify_all();
  }

  Results Get(PromiseState state, std::size_t limit)
  {
    FETCH_LOCK(mutex_);
    Results results;

    auto &worklist_for_state = workload_[state];

    if (worklist_for_state.size() <= limit)
    {
      results = std::move(worklist_for_state);
      worklist_for_state.clear();
    }
    else
    {
      std::copy_n(worklist_for_state.begin(), limit, std::inserter(results, results.begin()));
      auto copy_end = worklist_for_state.begin();
      advance(copy_end, static_cast<int64_t>(limit));
      worklist_for_state.erase(worklist_for_state.begin(), copy_end);
    }
    return results;
  }

  Results GetFailures(std::size_t limit)
  {
    return Get(PromiseState::FAILED, limit);
  }

  Results GetSuccesses(std::size_t limit)
  {
    return Get(PromiseState::SUCCESS, limit);
  }

  Results GetTimeouts(std::size_t limit)
  {
    return Get(PromiseState::TIMEDOUT, limit);
  }

  std::size_t CountPending()
  {
    FETCH_LOCK(mutex_);
    return workload_[PromiseState::WAITING].size();
  }

  std::size_t CountCompleted()
  {
    FETCH_LOCK(mutex_);
    return workload_[PromiseState::SUCCESS].size() + workload_[PromiseState::TIMEDOUT].size() +
           workload_[PromiseState::FAILED].size();
  }

  std::size_t CountSuccesses()
  {
    FETCH_LOCK(mutex_);
    return workload_[PromiseState::SUCCESS].size();
  }

  std::size_t CountFailures()
  {
    FETCH_LOCK(mutex_);
    return workload_[PromiseState::FAILED].size();
  }

  void DiscardFailures()
  {
    FETCH_LOCK(mutex_);
    workload_[PromiseState::FAILED].clear();
  }

  void DiscardSuccesses()
  {
    FETCH_LOCK(mutex_);
    workload_[PromiseState::FAILED].clear();
  }

  void DiscardTimeouts()
  {
    FETCH_LOCK(mutex_);
    workload_[PromiseState::TIMEDOUT].clear();
  }

  std::size_t CountTimeouts()
  {
    FETCH_LOCK(mutex_);
    return workload_[PromiseState::TIMEDOUT].size();
  }

  void Add(std::shared_ptr<WORKER> new_work)
  {
    {
      FETCH_LOCK(mutex_);
      // TODO(kll): use a no-copy insert operator here..
      workload_[PromiseState::WAITING].push_back(new_work);
    }
    Wake();
  }

  void Add(std::vector<std::shared_ptr<WORKER>> new_works)
  {
    {
      FETCH_LOCK(mutex_);
      // TODO(kll): use bulk insert operators here..
      for (auto new_work : new_works)
      {
        workload_[PromiseState::WAITING].push_back(new_works);
      }
    }
    Wake();
  }

  template <class KEY>
  bool Cancel(KEY const &key)
  {
    bool r = false;

    for (auto const &current_state : fetch::service::GetAllPromiseStates())
    {
      FETCH_LOCK(mutex_);
      auto &worklist_for_state = workload_[current_state];
      auto  workitem_iter      = worklist_for_state.begin();
      while (workitem_iter != worklist_for_state.end())
      {
        auto workitem = *workitem_iter;
        if (!workitem)
        {
          workitem_iter = worklist_for_state.erase(workitem_iter);
          continue;
        }
        if (workitem.Equals(key))
        {
          workitem_iter = worklist_for_state.erase(workitem_iter);
          r             = true;
          continue;
        }
        ++workitem_iter;
      }
    }
    return r;
  }

private:
  WorkLoad                workload_;
  mutable std::mutex      mutex_;
  std::condition_variable cv_;
};

}  // namespace network
}  // namespace fetch
