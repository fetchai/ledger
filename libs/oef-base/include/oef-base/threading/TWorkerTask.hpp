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

#include "WorkloadState.hpp"
#include "logging/logging.hpp"
#include "oef-base/threading/Notification.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Waitable.hpp"
#include "oef-base/threading/WorkloadState.hpp"

#include <algorithm>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <utility>

namespace fetch {
namespace oef {
namespace base {

template <class WORKLOAD>
class TWorkerTask : public Task
{
public:
  using Workload   = WORKLOAD;
  using WorkloadP  = std::shared_ptr<Workload>;
  using WaitableP  = std::shared_ptr<Waitable>;
  using QueueEntry = std::pair<WorkloadP, WaitableP>;
  using Queue      = std::queue<QueueEntry>;

  static constexpr char const *LOGGING_NAME = "TWorkerTask";

  using Mutex = std::mutex;
  using Lock  = std::lock_guard<Mutex>;

  TWorkerTask()
  {}
  virtual ~TWorkerTask()
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
      if (!current.first)
      {
        Lock lock(mutex);
        if (queue.empty())
        {
          FETCH_LOG_INFO(LOGGING_NAME, "No work, TWorkerTask sleeps");
          return DEFER;
        }
        FETCH_LOG_INFO(LOGGING_NAME, "TWorkerTask gets from queue");
        state = WorkloadState::START;
        std::swap(current.first, queue.front().first);
        current.second.swap(queue.front().second);
        queue.pop();
      }
      else
      {
        if (last_result == WorkloadProcessed::NOT_STARTED)
        {
          state = WorkloadState::START;
        }
        else
        {
          state = WorkloadState::RESUME;
        }
      }

      FETCH_LOG_INFO(LOGGING_NAME, "working...");
      auto result = process(current.first, state);
      last_result = result;
      FETCH_LOG_INFO(LOGGING_NAME, "Reply was ", workloadProcessedNames[result]);

      switch (result)
      {
      case WorkloadProcessed ::COMPLETE:
      {
        Lock lock(mutex);
        current.second->wake();
        current.first.reset();
      }
      break;
      case WorkloadProcessed::NOT_COMPLETE:
        return DEFER;
      case WorkloadProcessed::NOT_STARTED:
        return DEFER;
      }
    }
  }

protected:
  QueueEntry        current;
  WorkloadProcessed last_result;

  Queue         queue;
  mutable Mutex mutex;

private:
  TWorkerTask(const TWorkerTask &other) = delete;
  TWorkerTask &operator=(const TWorkerTask &other)  = delete;
  bool         operator==(const TWorkerTask &other) = delete;
  bool         operator<(const TWorkerTask &other)  = delete;
};

// namespace std { template<> void swap(TWorkerTask& lhs, TWorkerTask& rhs) { lhs.swap(rhs); } }
// std::ostream& operator<<(std::ostream& os, const TWorkerTask &output) {}
}  // namespace base
}  // namespace oef
}  // namespace fetch
