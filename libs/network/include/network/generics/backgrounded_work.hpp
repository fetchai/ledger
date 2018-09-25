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
template<class WORKER>
class BackgroundedWork
{
public:

  using WorkerWeak   = std::weak_ptr<WORKER>;
  using Worker       = std::shared_ptr<WORKER>;
  using PromiseState = fetch::service::PromiseState;
  using WorkLoad     = std::map<PromiseState, std::list<WorkerWeak>>;
  using Mutex        = fetch::mutex::Mutex;
  using Lock         = std::lock_guard<Mutex>;
  using CondVar      = std::condition_variable;
  using Results      = std::list<WorkerWeak>;
  
  // Construction / Destruction
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
    shutdown_.store(true);
    cv_.notify_all();
    cv_.notify_all();
    cv_.notify_all();
    if (thread_)
    {
      thread_->join();
    }
  }

  void WorkCycle()
  {
    FETCH_LOCK(mutex_);
    auto &worklist_for_state = workload_[PromiseState::WAITING];
    auto workitem_iter = worklist_for_state.begin();
    while(workitem_iter != worklist_for_state.end())
    {
      if (shutdown_.load())
      {
        return;
      }
      auto workitem_wp = *workitem_iter;
      auto workitem = workitem_wp.lock();
      if (!workitem)
      {
        workitem_iter = worklist_for_state.erase( workitem_iter );
      }
      else
      {
        try
        {
          auto r = workitem -> Work();
          switch(r)
          {
          case PromiseState::WAITING:
            ++workitem_iter;
            break;
          case PromiseState::SUCCESS:
          case PromiseState::FAILED:
          case PromiseState::TIMEDOUT:
            workload_[r].push_back(workitem_wp);
            workitem_iter = worklist_for_state.erase( workitem_iter );
            break;
          }
        }
        catch(std::exception ex)
        {
          // report exception here?
        }
      }
    }
  }

  void AutoWork()
  {
    thread_ = std::make_shared<std::thread>([this]() { this->AutoLoop(); });
  }

  void AutoLoop()
  {

    while(1)
    {
      if (shutdown_.load()) return;
      {
        Lock lock(mutex_);
        cv_.wait_for(std::chrono::milliseconds(100));
      }
      if (shutdown_.load()) return;
      WorkCycle();
      if (shutdown_.load()) return;
    }
  }

  Results Get(PromiseState state, std::size_t limit)
  {
    FETCH_LOCK(mutex_);
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
      // TODO
      //worklist_for_state.erase(worklist_for_state.begin(),
      //                         worklist_for_state.begin() + static_cast<std::ptrdiff_t>(limit));
    }
    return results;
  }

  size_t CountPending()
  {
    FETCH_LOCK(mutex_);
    return workload_[PromiseState::WAITING].size();
  }

  size_t CountCompleted()
  {
    FETCH_LOCK(mutex_);
    return
      workload_[PromiseState::SUCCESS].size()
      +
      workload_[PromiseState::TIMEDOUT].size()
      +
      workload_[PromiseState::FAILED].size()
      ;
  }

  size_t CountSuccesses()
  {
    FETCH_LOCK(mutex_);
    return workload_[PromiseState::SUCCESS].size();
  }

  size_t CountFailures()
  {
    FETCH_LOCK(mutex_);
    return workload_[PromiseState::FAILED].size();
  }

  size_t CountTimeouts()
  {
    FETCH_LOCK(mutex_);
    return workload_[PromiseState::TIMEDOUT].size();
  }

  void Add(std::shared_ptr<WORKER> new_work)
  {
    FETCH_LOCK(mutex_);
    std::weak_ptr<WORKER> wp(new_work);
    workload_[PromiseState::WAITING].push_back(wp);
    cv_.notify_one();
  }

  template<class KEY>
  bool InFlight(const KEY &key) const
  {
    FETCH_LOCK(mutex_);

    PromiseState promiseStates[] = {
      PromiseState::WAITING, PromiseState::SUCCESS, PromiseState::FAILED, PromiseState::TIMEDOUT
    };

    for(int i=0;i<4;i++)
    {
      auto current_state = promiseStates[i];
      auto &worklist_for_state = workload_[current_state];
      auto workitem_iter = worklist_for_state.begin();
      while(workitem_iter != worklist_for_state.end())
      {
        auto workitem_wp = *workitem_iter;
        auto workitem = workitem_wp.lock();
        if (!workitem)
        {
          workitem_iter = worklist_for_state.erase( workitem_iter );
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

  template<class KEY>
  bool Cancel(const KEY &key)
  {
    bool r = false;

    PromiseState promiseStates[] = {
      PromiseState::WAITING, PromiseState::SUCCESS, PromiseState::FAILED, PromiseState::TIMEDOUT
    };

    for(int i=0;i<4;i++)
    {
      FETCH_LOCK(mutex_);
      auto current_state = promiseStates[i];
      auto &worklist_for_state = workload_[current_state];
      auto workitem_iter = worklist_for_state.begin();
      while(workitem_iter != worklist_for_state.end())
      {
        auto workitem_wp = *workitem_iter;
        auto workitem = workitem_wp.lock();
        if (!workitem)
        {
          workitem_iter = worklist_for_state.erase( workitem_iter );
          continue;
        }
        if (workitem.Equals(key))
        {
          workitem_iter = worklist_for_state.erase( workitem_iter );
          r = true;
          continue;
        }
        ++workitem_iter;
      }
    }
    return r;
  }

private:
  WorkLoad                     workload_;
  Mutex                        mutex_{__LINE__, __FILE__};
  CondVar                      cv_;
  std::shared_ptr<std::thread> thread_;
  std::atomic<bool>            shutdown_{false};
};

}  // namespace network
}  // namespace fetch
