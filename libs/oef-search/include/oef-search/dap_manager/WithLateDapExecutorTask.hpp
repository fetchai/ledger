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
#include <queue>
#include <utility>

#include "Node.hpp"
#include "NodeExecutorTask.hpp"
#include "logging/logging.hpp"
#include "oef-base/threading/StateMachineTask.hpp"

class DapManager;

class WithLateDapExecutorTask
  : virtual public fetch::oef::base::StateMachineTask<WithLateDapExecutorTask>,
    virtual public NodeExecutorTask
{
public:
  using StateResult = typename fetch::oef::base::StateMachineTask<WithLateDapExecutorTask>::Result;
  using EntryPoint =
      typename fetch::oef::base::StateMachineTask<WithLateDapExecutorTask>::EntryPoint;
  using MessageHandler = std::function<void(std::shared_ptr<IdentifierSequence>)>;
  using ErrorHandler =
      std::function<void(const std::string &, const std::string &, const std::string &)>;
  using Parent = StateMachineTask<WithLateDapExecutorTask>;

  static constexpr char const *LOGGING_NAME = "WithLateDapExecutorTask";

  WithLateDapExecutorTask(std::shared_ptr<NodeExecutorTask> task,
                          std::vector<Node::DapMemento>     late_mementos,
                          std::shared_ptr<DapManager>       dap_manager)
    : Parent()
    , main_task_{std::move(task)}
    , late_mementos_{std::move(late_mementos)}
    , last_output_{nullptr}
    , dap_manager_{std::move(dap_manager)}
  {
    entryPoint.push_back(&WithLateDapExecutorTask::Setup);
    entryPoint.push_back(&WithLateDapExecutorTask::DoLateMementos);
    entryPoint.push_back(&WithLateDapExecutorTask::Done);
    this->entrypoints = entryPoint.data();
    this->state       = this->entrypoints[0];
    this->SetSubClass(this);
    FETCH_LOG_INFO(LOGGING_NAME, "Task created, id=", this->GetTaskId());
  }

  ~WithLateDapExecutorTask() override
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone, id=", this->GetTaskId());
  }

  WithLateDapExecutorTask(const WithLateDapExecutorTask &other) = delete;
  WithLateDapExecutorTask &operator=(const WithLateDapExecutorTask &other)  = delete;
  bool                     operator==(const WithLateDapExecutorTask &other) = delete;
  bool                     operator<(const WithLateDapExecutorTask &other)  = delete;

  MessageHandler GetMessageHandler(std::size_t const &task_id)
  {
    auto this_sp = this->template shared_from_base<WithLateDapExecutorTask>();
    std::weak_ptr<WithLateDapExecutorTask> this_wp = this_sp;
    auto                                   id      = this->GetTaskId();

    return [this_wp, id, task_id](std::shared_ptr<IdentifierSequence> response) {
      auto sp = this_wp.lock();
      if (sp)
      {
        sp->task_done.store(true);
        sp->last_output_ = std::move(response);
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to WithLateDapExecutorTask(", id,
                        ")! Called from task ", task_id);
      }
    };
  }

  ErrorHandler GetErrorHandler(std::size_t const &task_id)
  {
    auto this_sp = this->template shared_from_base<WithLateDapExecutorTask>();
    std::weak_ptr<WithLateDapExecutorTask> this_wp = this_sp;
    auto                                   id      = this->GetTaskId();

    return [this_wp, id, task_id](const std::string &dap_name, const std::string &path,
                                  const std::string &msg) {
      auto sp = this_wp.lock();
      if (sp)
      {
        sp->task_done.store(true);
        sp->last_output_ = nullptr;
        if (sp->errorHandler)
        {
          sp->errorHandler(dap_name, path, msg);
          sp->wake();
        }
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to WithLateDapExecutorTask(", id,
                        ")! Called from task ", task_id);
      }
    };
  }

  StateResult Setup()
  {
    main_task_->SetMessageHandler(GetMessageHandler(main_task_->GetTaskId()));
    main_task_->SetErrorHandler(GetErrorHandler(main_task_->GetTaskId()));
    task_done.store(false);
    main_task_->submit();

    auto this_sp = this->template shared_from_base<WithLateDapExecutorTask>();
    std::weak_ptr<WithLateDapExecutorTask> this_wp = this_sp;

    if (main_task_->MakeNotification()
            .Then([this_wp]() {
              auto sp = this_wp.lock();
              if (sp)
              {
                sp->MakeRunnable();
              }
            })
            .Waiting())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Sleeping (id=", this->GetTaskId(), "), will be woken by task ",
                     main_task_->GetTaskId());
      return StateResult(1, fetch::oef::base::DEFER);
    }

    FETCH_LOG_INFO(LOGGING_NAME, "NOT Sleeping id=(", this->GetTaskId(), ")");
    return StateResult(1, fetch::oef::base::COMPLETE);
  }

  StateResult DoLateMementos()
  {
    if (!task_done.load())
    {
      FETCH_LOG_INFO(LOGGING_NAME,
                     "Spurious wakeup in DoLateMementos(). Sleeping (id=", this->GetTaskId(), ")");
      return StateResult(1, fetch::oef::base::DEFER);
    }
    if (!last_output_)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "No last output set (id=", this->GetTaskId(), ")");
      wake();
      return StateResult(0, fetch::oef::base::ERRORED);
    }

    auto task = std::make_shared<MementoExecutorTask>(late_mementos_, last_output_, dap_manager_);

    task->SetMessageHandler(GetMessageHandler(task->GetTaskId()));

    task->SetErrorHandler(GetErrorHandler(task->GetTaskId()));
    task_done.store(false);
    task->submit();

    auto this_sp = this->template shared_from_base<WithLateDapExecutorTask>();
    std::weak_ptr<WithLateDapExecutorTask> this_wp = this_sp;

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
      FETCH_LOG_INFO(LOGGING_NAME, "Sleeping (id=", this->GetTaskId(),
                     ", do late mementos), will be woken by task ", task->GetTaskId());
      return StateResult(2, fetch::oef::base::DEFER);
    }

    FETCH_LOG_INFO(LOGGING_NAME, "NOT Sleeping id=(", this->GetTaskId(), ")");
    return StateResult(2, fetch::oef::base::COMPLETE);
  }

  StateResult Done()
  {
    if (!task_done.load())
    {
      FETCH_LOG_INFO(LOGGING_NAME, "Spurious wakeup in Done(). Sleeping (id=", this->GetTaskId(),
                     ")");
      return StateResult(2, fetch::oef::base::DEFER);
    }
    if (messageHandler)
    {
      messageHandler(last_output_);
    }
    wake();
    return StateResult(0, fetch::oef::base::COMPLETE);
  }

  void SetMessageHandler(MessageHandler msgHandler) override
  {
    messageHandler = std::move(msgHandler);
  }

  void SetErrorHandler(ErrorHandler errHandler) override
  {
    errorHandler = std::move(errHandler);
  }

public:
  ErrorHandler   errorHandler;
  MessageHandler messageHandler;

protected:
  std::shared_ptr<NodeExecutorTask>   main_task_;
  std::vector<Node::DapMemento>       late_mementos_;
  std::shared_ptr<IdentifierSequence> last_output_;
  std::shared_ptr<DapManager>         dap_manager_;
  std::atomic<bool>                   task_done{false};

  std::vector<EntryPoint> entryPoint;
};
