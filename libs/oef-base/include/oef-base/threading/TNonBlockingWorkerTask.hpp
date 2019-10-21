#pragma once

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

  TNonBlockingWorkerTask()          = default;
  virtual ~TNonBlockingWorkerTask() = default;

  Notification::NotificationBuilder post(WorkloadP workload)
  {
    Lock       lock(mutex);
    FETCH_LOG_INFO(LOGGING_NAME, "Adding workload for id=", workload->GetId());
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
      try {

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
          FETCH_LOG_INFO(LOGGING_NAME, "working (id=", it->first->GetId(),")...");
          state = not_started.find(*it) != not_started.end() ? WorkloadState::START
                                                             : WorkloadState::RESUME;
          auto result = process(it->first, state);
          FETCH_LOG_INFO(LOGGING_NAME, "Reply was (id=", it->first->GetId(), ")",
                         workloadProcessedNames[static_cast<int>(result)]);

          switch (result)
          {
            case WorkloadProcessed::COMPLETE:
            {
              it->second->wake();
              not_started.erase(*it);
              it = current.erase(it);
              break;
            }
            case WorkloadProcessed::NOT_COMPLETE:
            {
              not_started.erase(*it);
              ++it;
              break;
            }
            case WorkloadProcessed::NOT_STARTED:
            {
              ++it;
              break;
            }
          }
        }

        // if there is no more work or we have more then N work on flight and we started all the work
        {
          Lock lock(mutex);
          if ((queue.empty() || current.size() >= N) && not_started.empty())
          {
            return DEFER;
          }
        }
      }
      catch (std::exception& e)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Exception in the worker loop: ", e.what());
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
