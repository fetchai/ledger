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
#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Taskpool.hpp"

namespace fetch {
namespace oef {
namespace base {

Counter created_count("mt-core.tasks.created");
Counter destroyed_count("mt-core.tasks.destroyed");

thread_local std::size_t thread_group_id;

std::atomic<std::size_t> id_counter(0);

Task::Task()
  : task_state_{TaskState ::NOT_PENDING}
  , cancelled_(false)
  , made_runnable_{0}
{
  created_count++;
  group_id_ = thread_group_id;
  task_id_  = id_counter++;
  // FETCH_LOG_INFO(LOGGING_NAME, "created task ", task_id, " in group ", group_id);
  // Counter(std::string("mt-core.taskgroup.")+std::to_string(group_id)+".created")++;
}

void Task::cancel()  // try and cancel running task.
{
  cancelled_ = true;
  if (pool_)
  {
    pool_->remove(shared_from_this());
  }
}

std::size_t Task::SetGroupId(std::size_t new_group_id)
{
  auto g = group_id_;
  if (group_id_ != new_group_id)
  {
    group_id_ = new_group_id;
    // FETCH_LOG_INFO(LOGGING_NAME, " task ", task_id, " moved from group ", g, " to group ",
    // group_id);
  }
  return g;
}

void Task::SetThreadGroupId(std::size_t new_group_id)
{
  thread_group_id = new_group_id;
}

ExitState Task::RunThunk()
{
  thread_group_id = group_id_;
  return run();
}

Task::~Task()
{
  destroyed_count++;
  // FETCH_LOG_INFO(LOGGING_NAME, "killed task ", task_id, " in group ", group_id);
  // Counter(std::string("mt-core.taskgroup.")+std::to_string(group_id)+".destroyed")++;
}

bool Task::submit(std::shared_ptr<Taskpool> const &p)
{
  if (pool_ == p)
  {
    Counter("mt-core.tasks.submit(pool).not-submitted.aleady-in-pool")++;
    return true;
  }

  if (pool_)
  {
    pool_->remove(shared_from_this());
    Counter("mt-core.tasks.submit(pool).remove-from-pool")++;
  }
  pool_ = p;
  pool_->submit(shared_from_this());
  return true;
}

bool Task::submit()
{
  auto x = Taskpool::GetDefaultTaskpool().lock();
  if (pool_ == x)
  {
    Counter("mt-core.tasks.submit().not-submitted.aleady-in-pool")++;
    return true;
  }
  if (x)
  {
    return this->submit(x);
  }

  Counter("mt-core.tasks.submit().not-submitted.no-default-taskpool")++;
  return false;
}

bool Task::MakeRunnable()
{
  bool status = false;
  if (pool_)
  {
    status = pool_->MakeRunnable(shared_from_this());
  }
  else
  {
    Counter("mt-core.tasks.MakeRunnable.no-taskpool")++;
  }
  made_runnable_++;
  return status;
}

bool Task::submit(std::shared_ptr<Taskpool> const &p, std::chrono::milliseconds const &delay)
{
  if (pool_ == p)
  {
    Counter("mt-core.tasks.submit(pool,delay).not-submitted.aleady-in-pool")++;
    return true;
  }
  if (pool_)
  {
    Counter("mt-core.tasks.submit(pool,delay).remove-from-pool")++;
    pool_->remove(shared_from_this());
  }
  pool_ = p;
  pool_->after(shared_from_this(), delay);
  return true;
}

bool Task::submit(const std::chrono::milliseconds &delay)
{
  auto x = Taskpool::GetDefaultTaskpool().lock();
  if (x)
  {
    return this->submit(x, delay);
  }
  Counter("mt-core.tasks.submit(delay).not-submitted.no-default-taskpool")++;
  return false;
}

}  // namespace base
}  // namespace oef
}  // namespace fetch
