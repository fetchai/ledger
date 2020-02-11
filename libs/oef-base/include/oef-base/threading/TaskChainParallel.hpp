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

#include <memory>
#include <mutex>
#include <queue>
#include <utility>

#include "ExitState.hpp"
#include "core/mutex.hpp"
#include "logging/logging.hpp"
#include "oef-base/threading/StateMachineTask.hpp"

namespace fetch {
namespace oef {
namespace base {
template <typename IN_PROTO, typename OUT_PROTO, typename TaskInputDataType, class TaskType>
class TaskChainParallel : virtual public StateMachineTask<
                              TaskChainParallel<IN_PROTO, OUT_PROTO, TaskInputDataType, TaskType>>,
                          virtual public Waitable
{
public:
  using StateResult    = typename StateMachineTask<TaskChainParallel>::Result;
  using EntryPoint     = typename StateMachineTask<TaskChainParallel>::EntryPoint;
  using MessageHandler = std::function<void(std::shared_ptr<OUT_PROTO>)>;
  using OutputMerger =
      std::function<std::shared_ptr<OUT_PROTO>(std::vector<std::shared_ptr<OUT_PROTO>> &)>;
  using ErrorHandler =
      std::function<void(const std::string &, const std::string &, const std::string &)>;
  using Parent                              = StateMachineTask<TaskChainParallel>;
  static constexpr char const *LOGGING_NAME = "TaskChainParallel";

  TaskChainParallel()
    : Parent()
  {
    entryPoint.push_back(&TaskChainParallel::progress);
    entryPoint.push_back(&TaskChainParallel::progress);
    this->entrypoints = entryPoint.data();
    this->state       = this->entrypoints[0];
    this->SetSubClass(this);
    FETCH_LOG_INFO(LOGGING_NAME, "Task created, id=", this->GetTaskId());
  }

  ~TaskChainParallel() override
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone, id=", this->GetTaskId());
  }

  TaskChainParallel(const TaskChainParallel &other) = delete;
  TaskChainParallel &operator=(const TaskChainParallel &other)  = delete;
  bool               operator==(const TaskChainParallel &other) = delete;
  bool               operator<(const TaskChainParallel &other)  = delete;

  virtual std::shared_ptr<TaskType> CreateTask(const TaskInputDataType &,
                                               std::shared_ptr<IN_PROTO>)    = 0;
  virtual std::shared_ptr<IN_PROTO> GetInputProto(const TaskInputDataType &) = 0;

  virtual void Add(TaskInputDataType task)
  {
    tasks_.push(std::move(task));
    num_of_tasks_ = tasks_.size();
  }

  void SetGlobalInput(std::shared_ptr<IN_PROTO> input)
  {
    global_input_ = std::move(input);
  }

  StateResult progress()
  {
    auto id = this->GetTaskId();
    {
      FETCH_LOCK(result_mutex_);
      if ((results_.size() + errored_tasks_) == num_of_tasks_)
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Task(", id, ") done!");
        if (messageHandler && outputMerger)
        {
          messageHandler(outputMerger(GetOutputs()));
        }
        else
        {
          FETCH_LOG_INFO(LOGGING_NAME, "No message handler in task ", id, "!");
        }
        wake();
        if (errored_tasks_ == 0)
        {
          return StateResult(0, COMPLETE);
        }

        return StateResult(0, ERRORED);
      }
    }
    auto                             this_sp = this->template shared_from_base<TaskChainParallel>();
    std::weak_ptr<TaskChainParallel> this_wp = this_sp;

    uint32_t counter = 0;
    while (!tasks_.empty())
    {
      auto data  = tasks_.front();
      auto input = global_input_;
      if (!input)
      {
        input = GetInputProto(data);
      }
      auto task = CreateTask(data, input);

      if (task == nullptr)
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "Failed to create task (id=", id, ")!");
        wake();
        return StateResult(0, ERRORED);
      }
      auto task_id = task->GetTaskId();
      FETCH_LOG_INFO(LOGGING_NAME, "Added task ", task_id, " to ", id);

      task->SetMessageHandler([this_wp, id, task_id](std::shared_ptr<OUT_PROTO> response) {
        auto sp = this_wp.lock();
        if (sp)
        {
          std::lock_guard<std::mutex> lock(sp->result_mutex_);
          sp->results_.push_back(std::move(response));
        }
        else
        {
          FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to TaskChainParallel(", id,
                          "), Called by task ", task_id);
        }
      });

      task->SetErrorHandler([this_wp, id, task_id](const std::string &dap_name,
                                                   const std::string &path,
                                                   const std::string &msg) {
        auto sp = this_wp.lock();
        if (sp)
        {
          {
            std::lock_guard<std::mutex> lock(sp->result_mutex_);
            ++(sp->errored_tasks_);
          }
          if (sp->errorHandler)
          {
            sp->errorHandler(dap_name, path, msg);
          }
        }
        else
        {
          FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to TaskChainParallel(", id,
                          "), Called by task ", task_id);
        }
      });

      task->submit();

      tasks_.pop();

      if (task->MakeNotification()
              .Then([this_wp]() {
                auto sp = this_wp.lock();
                if (sp)
                {
                  sp->MakeRunnable();
                }
              })
              .Waiting())
      {
        ++counter;
      }
    }
    // spurious wakeup handled as well
    if ((results_.size() + errored_tasks_) < num_of_tasks_)
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Sleeping (id=", this->GetTaskId(), ")");
      return StateResult(1, DEFER);
    }

    FETCH_LOG_INFO(LOGGING_NAME, "Done. (id=", this->GetTaskId(), ")");
    return StateResult(1, COMPLETE);
  }

  std::vector<std::shared_ptr<OUT_PROTO>> &GetOutputs()
  {
    return results_;
  }

public:
  ErrorHandler   errorHandler;
  MessageHandler messageHandler;
  OutputMerger   outputMerger;

protected:
  std::shared_ptr<IN_PROTO>               global_input_ = nullptr;
  std::queue<TaskInputDataType>           tasks_{};
  std::size_t                             num_of_tasks_ = 0;
  std::mutex                              result_mutex_;
  std::vector<std::shared_ptr<OUT_PROTO>> results_{};
  uint32_t                                errored_tasks_ = 0;

  std::vector<EntryPoint> entryPoint;
};
}  // namespace base
}  // namespace oef
}  // namespace fetch
