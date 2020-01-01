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

#include <atomic>
#include <chrono>
#include <memory>

#include "oef-base/threading/ExitState.hpp"

namespace fetch {
namespace oef {
namespace base {

class Taskpool;

class Task : public std::enable_shared_from_this<Task>
{
  friend class Taskpool;

public:
  static constexpr char const *LOGGING_NAME = "Task";

  enum TaskState
  {
    NOT_PENDING,
    PENDING,
    SUSPENDED,
    DONE,
  };

  virtual bool      IsRunnable() const = 0;
  virtual ExitState RunThunk();
  virtual ExitState run() = 0;
  virtual void      cancel();

  bool IsCancelled() const
  {
    return cancelled_;
  }

  Task();
  virtual ~Task();

  bool submit(std::shared_ptr<Taskpool> const &pool, std::chrono::milliseconds const &delay);
  bool submit(std::chrono::milliseconds const &delay);
  bool submit(std::shared_ptr<Taskpool> const &pool);
  bool submit();

  virtual bool MakeRunnable();

  virtual uint16_t GetMadeRunnableCountAndClear()
  {
    return made_runnable_.exchange(0);
  }

  std::size_t GetTaskId()
  {
    return task_id_;
  }
  std::size_t GetGroupId()
  {
    return group_id_;
  }

  std::size_t SetGroupId(std::size_t new_group_id);
  static void SetThreadGroupId(std::size_t new_group_id);

  template <typename Derived>
  std::shared_ptr<Derived> shared_from_base()
  {
    return std::dynamic_pointer_cast<Derived>(shared_from_this());
  }

  void SetTaskState(TaskState state)
  {
    task_state_.store(state);
  }

  TaskState GetTaskState() const
  {
    return task_state_.load();
  }

private:
  std::shared_ptr<Taskpool> pool_;
  std::atomic<TaskState>    task_state_;
  std::atomic<bool>         cancelled_;
  std::size_t               group_id_;
  std::size_t               task_id_;
  std::atomic<uint16_t>     made_runnable_;
};

}  // namespace base
}  // namespace oef
}  // namespace fetch
