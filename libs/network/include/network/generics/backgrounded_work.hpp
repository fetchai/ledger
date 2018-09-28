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

namespace fetch {
namespace network {

/**
 * Simple wrapper around the service promise which mandates the return time from the underlying
 * promise.
 *
 * @tparam RESULT The expected return type of the promise
 */
template <class WORKER>
class BackgroundedWork
{
public:
  using Worker       = std::shared_ptr<WORKER>;
  using PromiseState = fetch::service::PromiseState;
  using WorkLoad     = std::map<PromiseState, std::list<Worker>>;
  using Mutex        = std::mutex;
  using Lock         = std::unique_lock<Mutex>;
  using Results      = std::list<Worker>;
  using CondVar      = std::condition_variable;

  // Construction / Destruction
  BackgroundedWork()
  {
    Lock lock(mutex_);
    workload_[PromiseState::WAITING];
    workload_[PromiseState::SUCCESS];
    workload_[PromiseState::FAILED];
    workload_[PromiseState::TIMEDOUT];
  }

  ~BackgroundedWork()
  {
    Lock lock(mutex_);
  }

  void WorkCycle()
  {
    Lock  lock(mutex_);
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
        try
        {
          auto r = workitem->Work();
          switch (r)
          {
          case PromiseState::WAITING:
            ++workitem_iter;
            break;
          case PromiseState::SUCCESS:
          case PromiseState::FAILED:
          case PromiseState::TIMEDOUT:
            workload_[r].push_back(workitem);
            workitem_iter = worklist_for_state.erase(workitem_iter);
            break;
          }
        }
        catch (std::exception ex)
        {
          // report exception here?
        }
      }
    }
  }

  void Wait(int milliseconds)
  {
    Lock lock(mutex_);
    cv_.wait_for(lock, std::chrono::milliseconds(milliseconds));
  }

  void Wake()
  {
    cv_.notify_one();
  }

  void WakeAll()
  {
    cv_.notify_all();
  }

  Results Get(PromiseState state, std::size_t limit)
  {
    Lock    lock(mutex_);
    Results results;

    auto &worklist_for_state = workload_[state];

    if (worklist_for_state.size() <= limit)
    {
      results = std::move(worklist_for_state);
      worklist_for_state.clear();  // needed?
    }
    else
    {
      std::copy_n(worklist_for_state.begin(), limit, std::inserter(results, results.begin()));
      auto copy_end = worklist_for_state.begin();
      advance(copy_end, static_cast<long>(limit));
      worklist_for_state.erase(worklist_for_state.begin(), copy_end);
    }
    return results;
  }

  size_t CountPending()
  {
    Lock lock(mutex_);
    return workload_[PromiseState::WAITING].size();
  }

  size_t CountCompleted()
  {
    Lock lock(mutex_);
    return workload_[PromiseState::SUCCESS].size() + workload_[PromiseState::TIMEDOUT].size() +
           workload_[PromiseState::FAILED].size();
  }

  size_t CountSuccesses()
  {
    Lock lock(mutex_);
    return workload_[PromiseState::SUCCESS].size();
  }

  size_t CountFailures()
  {
    Lock lock(mutex_);
    return workload_[PromiseState::FAILED].size();
  }

  void DiscardFailures()
  {
    Lock lock(mutex_);
    workload_[PromiseState::FAILED].clear();
  }

  void DiscardTimeouts()
  {
    Lock lock(mutex_);
    workload_[PromiseState::TIMEDOUT].clear();
  }

  size_t CountTimeouts()
  {
    Lock lock(mutex_);
    return workload_[PromiseState::TIMEDOUT].size();
  }

  void Add(std::shared_ptr<WORKER> new_work)
  {
    Lock lock(mutex_);
    workload_[PromiseState::WAITING].push_back(new_work);
    Wake();
  }

  void Add(std::vector<std::shared_ptr<WORKER>> new_works)
  {
    Lock lock(mutex_);
    // TODO(kll) use bulk insert operators here..
    for (auto new_work : new_works)
    {
      workload_[PromiseState::WAITING].push_back(new_works);
    }
    Wake();
  }

  void Add(std::list<std::shared_ptr<WORKER>> new_works)
  {
    Lock lock(mutex_);
    // TODO(kll) use bulk insert operators here..
    for (auto new_work : new_works)
    {
      std::weak_ptr<WORKER> wp(new_work);
      workload_[PromiseState::WAITING].push_back(wp);
    }
    Wake();
  }

  template <class KEY>
  bool InFlight(const KEY &key) const
  {
    Lock lock(mutex_);

    PromiseState promiseStates[] = {PromiseState::WAITING, PromiseState::SUCCESS,
                                    PromiseState::FAILED, PromiseState::TIMEDOUT};

    for (int i = 0; i < 4; i++)
    {
      auto  current_state      = promiseStates[i];
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
          return true;
        }
        ++workitem_iter;
      }
    }
    return false;
  }

  template <class KEY>
  bool Cancel(const KEY &key)
  {
    bool r = false;

    PromiseState promiseStates[] = {PromiseState::WAITING, PromiseState::SUCCESS,
                                    PromiseState::FAILED, PromiseState::TIMEDOUT};

    for (int i = 0; i < 4; i++)
    {
      Lock  lock(mutex_);
      auto  current_state      = promiseStates[i];
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
  WorkLoad workload_;
  Mutex    mutex_;  //{__LINE__, __FILE__};
  CondVar  cv_;
};

}  // namespace network
}  // namespace fetch
