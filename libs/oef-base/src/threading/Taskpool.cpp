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

#include "logging/logging.hpp"
#include "oef-base/monitoring/Counter.hpp"
#include "oef-base/monitoring/Gauge.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Taskpool.hpp"

#include <algorithm>

namespace fetch {
namespace oef {
namespace base {

static std::weak_ptr<Taskpool> gDefaultTaskPool;

static Gauge gauge_pending("mt-core.taskpool.gauge.runnable_tasks");
static Gauge gauge_running("mt-core.taskpool.gauge.running_tasks");
static Gauge gauge_suspended("mt-core.taskpool.gauge.sleeping_tasks");
static Gauge gauge_future("mt-core.taskpool.gauge.future_tasks");

Taskpool::Taskpool()
  : quit_(false)
{
  Counter("mt-core.tasks.popped-for-run");
  Counter("mt-core.tasks.run.std::exception");
  Counter("mt-core.tasks.run.exception");
  Counter("mt-core.tasks.run.deferred");
  Counter("mt-core.tasks.run.errored");
  Counter("mt-core.tasks.run.cancelled");
  Counter("mt-core.tasks.run.completed");
}

void Taskpool::SetDefault()
{
  gDefaultTaskPool = shared_from_this();
}

std::weak_ptr<Taskpool> Taskpool::GetDefaultTaskpool()
{
  return gDefaultTaskPool;
}

void Taskpool::run(std::size_t thread_idx)
{
  while (true)
  {
    if (quit_)
    {
      return;
    }

    Timestamp now = Clock::now();
    {
      Lock lock(mutex_);
      if (pending_tasks_.empty())
      {
        work_available_.wait_until(lock, lockless_getNextWakeTime(now, Milliseconds(100)));
      }
    }

    if (quit_)
    {
      return;
    }

    TaskP mytask;

    now = Clock::now();

    {
      Lock lock(mutex_);
      mytask = lockless_getNextFutureWork(now);
    }

    if (!mytask)
    {
      Lock lock(mutex_);
      if (!pending_tasks_.empty())
      {
        mytask        = pending_tasks_.front();
        mytask->pool_ = nullptr;
        pending_tasks_.pop_front();
        Counter("mt-core.tasks.popped-for-run")++;
        Counter("mt-core.immediate-tasks.popped-for-run")++;
      }
    }

    if (!mytask)
    {
      continue;
    }

    ExitState status;

    {
      Lock lock(mutex_);
      mytask->SetTaskState(Task::TaskState::NOT_PENDING);
      running_tasks_[thread_idx] = mytask;
    }

    try
    {
      if (mytask->IsCancelled())
      {
        status = CANCELLED;
      }
      else
      {
        status = mytask->RunThunk();
      }
    }
    catch (std::exception const &ex)
    {
      Counter("mt-core.tasks.run.std::exception")++;
      FETCH_LOG_INFO(LOGGING_NAME, "Threadpool caught:", ex.what());
      status = ERRORED;
    }
    catch (...)
    {
      Counter("mt-core.tasks.run.exception")++;
      FETCH_LOG_INFO(LOGGING_NAME, "Threadpool caught: other exception");
      status = ERRORED;
    }

    {
      Lock lock(mutex_);
      running_tasks_.erase(thread_idx);
    }

    switch (status)
    {
    case DEFER:
    {
      if (mytask->GetMadeRunnableCountAndClear() == 0)
      {
        Counter("mt-core.tasks.run.deferred")++;
        suspend(mytask);
      }
      else
      {
        Counter("mt-core.tasks.run.deferred.rerun")++;
        submit(mytask);
      }
      break;
    }
    case ERRORED:
    {
      Counter("mt-core.tasks.run.errored")++;
      Lock lock(mutex_);
      mytask->SetTaskState(Task::TaskState::DONE);
      mytask.reset();
      break;
    }
    case CANCELLED:
    {
      Counter("mt-core.tasks.run.cancelled")++;
      Lock lock(mutex_);
      mytask->SetTaskState(Task::TaskState::DONE);
      mytask.reset();
      break;
    }
    case COMPLETE:
    {
      Counter("mt-core.tasks.run.completed")++;
      Lock lock(mutex_);
      mytask->SetTaskState(Task::TaskState::DONE);
      mytask.reset();
      break;
    }
    case RERUN:
    {
      Counter("mt-core.tasks.run.rerun")++;
      submit(mytask);
      break;
    }
    }
  }
}

void Taskpool::remove(TaskP task)
{
  Lock lock(mutex_);
  task->SetTaskState(Task::TaskState::DONE);
  bool did = false;
  {
    auto iter = pending_tasks_.begin();
    while (iter != pending_tasks_.end())
    {
      if (*iter == task)
      {
        iter = pending_tasks_.erase(iter);
        Counter("mt-core.tasks.removed.runnable")++;
        did = true;
      }
      else
      {
        ++iter;
      }
    }
  }
  {
    auto iter = suspended_tasks_.find(task);
    if (iter != suspended_tasks_.end())
    {
      suspended_tasks_.erase(iter);
      did = true;
      Counter("mt-core.tasks.removed.sleeping")++;
    }
  }
  if (!did)
  {
    Counter("mt-core.tasks.removed.notfound")++;
  }
}

bool Taskpool::MakeRunnable(TaskP task)
{
  Lock lock(mutex_);
  bool status = false;
  auto iter   = suspended_tasks_.find(task);
  if (iter != suspended_tasks_.end())
  {
    Counter("mt-core.tasks.made-runnable")++;
    auto t = *iter;
    t->SetTaskState(Task::TaskState::PENDING);
    suspended_tasks_.erase(iter);
    pending_tasks_.push_front(t);
    work_available_.notify_one();
    status = true;
  }
  else
  {
    Counter("mt-core.tasks.made-runnable.failed")++;
    bool in_pending =
        std::find(pending_tasks_.begin(), pending_tasks_.end(), task) != pending_tasks_.end();
    bool in_running = false;
    for (const auto &e : running_tasks_)
    {
      if (e.second->GetTaskId() == task->GetTaskId())
      {
        in_running = true;
        break;
      }
    }
    FETCH_LOG_WARN(LOGGING_NAME, "Task ", task->GetTaskId(),
                   " not in suspended_tasks list! in_pending=", in_pending,
                   ", in_running=", in_running);
  }
  return status;
}

void Taskpool::UpdateStatus() const
{
  Lock lock(mutex_);
  gauge_pending   = pending_tasks_.size();
  gauge_running   = running_tasks_.size();
  gauge_suspended = suspended_tasks_.size();
  gauge_future    = future_tasks_.size();
}

void Taskpool::stop()
{
  Lock lock(mutex_);
  quit_ = true;

  for (auto const &t : pending_tasks_)
  {
    t->SetTaskState(Task::TaskState::DONE);
    t->cancel();
  }
  pending_tasks_.clear();

  for (auto const &kv : running_tasks_)
  {
    auto t = kv.second;
    if (t)
    {
      t->cancel();
    }
  }

  work_available_.notify_all();
  work_available_.notify_all();
  work_available_.notify_all();
}

void Taskpool::suspend(TaskP task)
{
  Counter("mt-core.tasks.suspended")++;
  Lock lock(mutex_);
  if (task->GetTaskState() == Task::TaskState::PENDING)
  {
    // somebody else moved task to pending list while we were waiting for the mutex
    FETCH_LOG_INFO(LOGGING_NAME, "Task ", task->GetTaskId(), " not suspended because is pending!");
    return;
  }
  task->SetTaskState(Task::TaskState::SUSPENDED);
  task->pool_ = shared_from_this();
  suspended_tasks_.insert(task);
}

void Taskpool::submit(TaskP task)
{
  if (task->IsRunnable())
  {
    Lock lock(mutex_);
    Counter("mt-core.tasks.moved-to-runnable")++;
    task->SetTaskState(Task::TaskState::PENDING);
    pending_tasks_.push_back(task);
    work_available_.notify_one();
  }
  else
  {
    suspend(task);
  }
}

void Taskpool::after(TaskP task, const Milliseconds &delay)
{
  Lock lock(mutex_);

  FutureTask ft;
  ft.task = task;
  ft.due  = Clock::now() + delay;

  task->SetTaskState(Task::TaskState::NOT_PENDING);
  future_tasks_.push(ft);
  Counter("mt-core.tasks.futured")++;
}

Taskpool::Timestamp Taskpool::lockless_getNextWakeTime(const Timestamp &   current_time,
                                                       const Milliseconds &deflt)
{
  Timestamp result = current_time + deflt;
  if (!future_tasks_.empty())
  {
    result = future_tasks_.top().due;
  }
  return result;
}

Taskpool::TaskP Taskpool::lockless_getNextFutureWork(const Timestamp &current_time)
{
  TaskP result;
  while ((!future_tasks_.empty()) && future_tasks_.top().due <= current_time)
  {
    auto r = future_tasks_.top().task;
    future_tasks_.pop();

    if (!(r->IsCancelled()))
    {
      result        = r;
      result->pool_ = nullptr;
      Counter("mt-core.tasks.popped-for-run")++;
      Counter("mt-core.future-tasks.popped-for-run")++;
      break;
    }
  }
  return result;
}

void Taskpool::CancelTaskGroup(std::size_t group_id)
{

  FETCH_LOG_INFO(LOGGING_NAME, "CancelTaskGroup ", group_id);

  std::list<TaskP> tasks;

  {
    Lock lock(mutex_);
    {
      auto iter = pending_tasks_.begin();
      while (iter != pending_tasks_.end())
      {
        if ((*iter)->group_id_ == group_id)
        {
          (*iter)->SetTaskState(Task::TaskState::DONE);
          tasks.push_back(*iter);
          iter = pending_tasks_.erase(iter);
        }
        else
        {
          ++iter;
        }
      }
    }

    {
      auto iter = suspended_tasks_.begin();
      while (iter != suspended_tasks_.end())
      {
        if ((*iter)->group_id_ == group_id)
        {
          (*iter)->SetTaskState(Task::TaskState::DONE);
          tasks.push_back(*iter);
          iter = suspended_tasks_.erase(iter);
        }
        else
        {
          ++iter;
        }
      }
    }
  }

  for (auto const &t : tasks)
  {
    FETCH_LOG_INFO(LOGGING_NAME, "CancelTaskGroup ", group_id, " (P) task ", t->task_id_);
    t->cancel();
  }
}

}  // namespace base
}  // namespace oef
}  // namespace fetch
