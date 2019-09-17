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

#include "logging/logging.hpp"
#include "oef-base/threading/Notification.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Waitable.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_set>
#include <utility>

template <class WORKLOAD, int N = 1>
class TNonBlockingWorkerTask : public Task
{
public:
  using Workload   = WORKLOAD;
  using WorkloadP  = std::shared_ptr<Workload>;
  using WaitableP  = std::shared_ptr<Waitable>;
  using QueueEntry = std::pair<WorkloadP, WaitableP>;
  using Queue      = std::queue<QueueEntry>;

  static constexpr char const *LOGGING_NAME = "TNonBlockingWorkerTask";

  using Mutex = std::mutex;
  using Lock  = std::lock_guard<Mutex>;

  using WorkloadProcessed = enum {
    COMPLETE,
    NOT_COMPLETE,
    NOT_STARTED,
  };

  using WorkloadState = enum { START, RESUME };

  TNonBlockingWorkerTask()
  {}
  virtual ~TNonBlockingWorkerTask()
  {}

  Notification::NotificationBuilder post(WorkloadP workload)
  {
    Lock       lock(mutex);
    WaitableP  waitable = std::make_shared<Waitable>();
    QueueEntry qe(workload, waitable);
    queue.push(qe);

    // now make this task runnable.

    this->MakeRunnable();

    return waitable->MakeNotification();
  }

  virtual WorkloadProcessed process(WorkloadP workload, WorkloadState state) = 0;

  virtual bool IsRunnable(void) const
  {
    Lock lock(mutex);
    return !queue.empty();
  }

  bool HasCurrentTask(void) const
  {
    Lock lock(mutex);
    return bool(current.first);
  }

  virtual ExitState run(void)
  {
    WorkloadState state;

    while (true)
    {

      {
        Lock lock(mutex);
        while (!queue.empty() && current.size() < N)
        {
          current.insert(queue.front());
          not_started.insert(queue.front());
          queue.pop();
        }
      }

      auto it = current.begin();
      while (it != current.end())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "working...");
        state       = not_started.find(*it) != not_started.end() ? START : RESUME;
        auto result = process(it->first, state);
        FETCH_LOG_INFO(LOGGING_NAME, "Reply was ", exitStateNames[result]);

        switch (result)
        {
        case COMPLETE:
        {
          it->second->wake();
          not_started.erase(*it);
          it = current.erase(it);
          break;
        }
        case NOT_COMPLETE:
        {
          not_started.erase(*it);
          ++it;
          break;
        }
        case NOT_STARTED:
        {
          ++it;
          break;
        }
        }
      }

      if (queue.empty() || current.size() >= N)
      {
        return DEFER;
      }
    }
  }

public:
  struct QueueEntryHash
  {
  public:
    std::size_t operator()(const QueueEntry &entry) const
    {
      return std::hash<WorkloadP>()(entry.first);
    }
  };

protected:
  std::unordered_set<QueueEntry, QueueEntryHash> current;
  std::unordered_set<QueueEntry, QueueEntryHash> not_started;

  Queue         queue;
  mutable Mutex mutex;

private:
  TNonBlockingWorkerTask(const TNonBlockingWorkerTask &other) = delete;
  TNonBlockingWorkerTask &operator=(const TNonBlockingWorkerTask &other)  = delete;
  bool                    operator==(const TNonBlockingWorkerTask &other) = delete;
  bool                    operator<(const TNonBlockingWorkerTask &other)  = delete;
};

// namespace std { template<> void swap(TWorkerTask& lhs, TWorkerTask& rhs) { lhs.swap(rhs); } }
// std::ostream& operator<<(std::ostream& os, const TWorkerTask &output) {}
