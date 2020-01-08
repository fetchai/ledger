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

#include <condition_variable>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <vector>

#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/Task.hpp"

namespace fetch {
namespace oef {
namespace base {

class Taskpool : public std::enable_shared_from_this<Taskpool>
{
public:
  static constexpr char const *LOGGING_NAME = "Taskpool";

  using Mutex = std::mutex;
  using Lock  = std::unique_lock<Mutex>;

  using TaskP          = std::shared_ptr<Task>;
  using Tasks          = std::list<TaskP>;
  using TaskDone       = std::pair<ExitState, TaskP>;
  using RunningTasks   = std::map<std::size_t, TaskP>;
  using SuspendedTasks = std::set<TaskP>;

  using Clock        = std::chrono::system_clock;
  using Timestamp    = Clock::time_point;
  using Milliseconds = std::chrono::milliseconds;

  Taskpool();
  virtual ~Taskpool() = default;

  virtual void submit(TaskP task);
  virtual void suspend(TaskP task);
  virtual void after(TaskP task, const Milliseconds &delay);

  virtual void run(std::size_t thread_idx);
  virtual void SetDefault();
  virtual void stop();

  static std::weak_ptr<Taskpool> GetDefaultTaskpool();

  virtual void remove(TaskP task);
  virtual bool MakeRunnable(TaskP task);

  struct TaskpoolStatus
  {
    std::size_t pending_tasks;
    std::size_t running_tasks;
    std::size_t suspended_tasks;
    std::size_t future_tasks;
  };

  void UpdateStatus() const;

  void CancelTaskGroup(std::size_t group_id);

  Taskpool(const Taskpool &other) = delete;
  Taskpool &operator=(const Taskpool &other)  = delete;
  bool      operator==(const Taskpool &other) = delete;
  bool      operator<(const Taskpool &other)  = delete;

protected:
private:
  Timestamp lockless_getNextWakeTime(const Timestamp &current_time, const Milliseconds &deflt);
  TaskP     lockless_getNextFutureWork(const Timestamp &current_time);

  mutable Mutex           mutex_;
  std::atomic<bool>       quit_;
  std::condition_variable work_available_;
  Tasks                   pending_tasks_;
  RunningTasks            running_tasks_;
  SuspendedTasks          suspended_tasks_;

  struct FutureTask
  {
    TaskP     task;
    Timestamp due;
  };

  struct FutureTaskOrdering
  {
    bool operator()(const FutureTask &a, const FutureTask &b)
    {
      return (a.due > b.due);
    }
  };

  using FutureTasks = std::priority_queue<FutureTask, std::vector<FutureTask>, FutureTaskOrdering>;

  FutureTasks future_tasks_;
};
}  // namespace base
}  // namespace oef
}  // namespace fetch
