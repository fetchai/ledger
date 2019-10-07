#pragma once

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
  virtual ~Taskpool();

  virtual void submit(TaskP task);
  virtual void suspend(TaskP task);
  virtual void after(TaskP task, const Milliseconds &delay);

  virtual void run(std::size_t thread_number);
  virtual void setDefault();
  virtual void stop(void);

  static std::weak_ptr<Taskpool> getDefaultTaskpool();

  virtual void remove(TaskP task);
  virtual void makeRunnable(TaskP task);

  struct TaskpoolStatus
  {
    std::size_t pending_tasks;
    std::size_t running_tasks;
    std::size_t suspended_tasks;
    std::size_t future_tasks;
  };

  void updateStatus() const;

  void cancelTaskGroup(std::size_t group_id);

protected:
private:
  Timestamp lockless_getNextWakeTime(const Timestamp &current_time, const Milliseconds &deflt);
  TaskP     lockless_getNextFutureWork(const Timestamp &current_time);

  mutable Mutex           mutex;
  std::atomic<bool>       quit;
  std::condition_variable work_available;
  Tasks                   pending_tasks;
  RunningTasks            running_tasks;
  SuspendedTasks          suspended_tasks;

  struct FutureTask
  {
    TaskP     task;
    Timestamp due;
  };

  struct FutureTaskOrdering
  {
    bool operator()(const FutureTask &a, const FutureTask &b)
    {
      // Needs to be a > because the PriQ outputs largest elements first.
      // so if a's due is before b's
      // we need to say "false".
      if (a.due < b.due)
      {
        return false;
      }
      else
      {
        return true;
      }
    }
  };

  using FutureTasks = std::priority_queue<FutureTask, std::vector<FutureTask>, FutureTaskOrdering>;

  FutureTasks future_tasks;

  Taskpool(const Taskpool &other) = delete;              // { copy(other); }
  Taskpool &operator=(const Taskpool &other)  = delete;  // { copy(other); return *this; }
  bool      operator==(const Taskpool &other) = delete;  // const { return compare(other)==0; }
  bool      operator<(const Taskpool &other)  = delete;  // const { return compare(other)==-1; }
};
