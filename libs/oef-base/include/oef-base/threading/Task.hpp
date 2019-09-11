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

#include <atomic>
#include <chrono>
#include <memory>

#include "oef-base/threading/ExitState.hpp"

class Taskpool;

class Task : public std::enable_shared_from_this<Task>
{
  friend class Taskpool;

public:
  static constexpr char const *LOGGING_NAME = "Task";

  virtual bool      isRunnable(void) const = 0;
  virtual ExitState runThunk(void);
  virtual ExitState run(void) = 0;
  virtual void      cancel(void);

  bool isCancelled(void) const
  {
    return cancelled;
  }

  Task();
  virtual ~Task();

  bool submit(std::shared_ptr<Taskpool> pool, const std::chrono::milliseconds &delay);
  bool submit(const std::chrono::milliseconds &delay);
  bool submit(std::shared_ptr<Taskpool> pool);
  bool submit();

  virtual void makeRunnable();

  std::size_t getTaskId()
  {
    return task_id;
  }

  std::size_t setGroupId(std::size_t new_group_id);
  static void setThreadGroupId(std::size_t new_group_id);

private:
  std::shared_ptr<Taskpool> pool;
  std::atomic<bool>         cancelled;
  std::size_t               group_id;
  std::size_t               task_id;
};
