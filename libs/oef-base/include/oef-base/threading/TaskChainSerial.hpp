#pragma once

#include <memory>
#include <queue>
#include <utility>

#include "logging/logging.hpp"
#include "oef-base/threading/StateMachineTask.hpp"

template <typename IN_PROTO, typename OUT_PROTO, typename PipeDataType, class TaskType>
class TaskChainSerial
  : virtual public StateMachineTask<TaskChainSerial<IN_PROTO, OUT_PROTO, PipeDataType, TaskType>>,
    virtual public Waitable
{
public:
  using StateResult = typename StateMachineTask<TaskChainSerial>::Result;
  using EntryPoint  = typename StateMachineTask<TaskChainSerial>::EntryPoint;
  using ProtoPipeBuilder =
      std::function<std::shared_ptr<IN_PROTO>(std::shared_ptr<OUT_PROTO>, const PipeDataType &)>;
  using MessageHandler = std::function<void(std::shared_ptr<OUT_PROTO>)>;
  using ErrorHandler =
      std::function<void(const std::string &, const std::string &, const std::string &)>;
  using TaskResultUpdate = std::function<std::shared_ptr<OUT_PROTO>(
      std::shared_ptr<TaskChainSerial>, std::shared_ptr<OUT_PROTO>)>;
  using Parent           = StateMachineTask<TaskChainSerial>;

  static constexpr char const *LOGGING_NAME = "TaskChainSerial";

  TaskChainSerial()
    : Parent()
  {
    entryPoint.push_back(&TaskChainSerial::progress);
    entryPoint.push_back(&TaskChainSerial::progress);
    this->entrypoints = entryPoint.data();
    this->state       = this->entrypoints[0];
    this->SetSubClass(this);
    FETCH_LOG_INFO(LOGGING_NAME, "Task created.");
  }

  virtual ~TaskChainSerial()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone.");
  }

  TaskChainSerial(const TaskChainSerial &other) = delete;
  TaskChainSerial &operator=(const TaskChainSerial &other)  = delete;
  bool             operator==(const TaskChainSerial &other) = delete;
  bool             operator<(const TaskChainSerial &other)  = delete;

  virtual std::shared_ptr<TaskType> CreateTask(const PipeDataType &, std::shared_ptr<IN_PROTO>) = 0;

  void SetPipeBuilder(ProtoPipeBuilder func)
  {
    protoPipeBuilder = func;
  }

  void InitPipe(std::shared_ptr<OUT_PROTO> init = nullptr)
  {
    if (init)
    {
      last_output = init;
    }
    else
    {
      last_output = std::make_shared<OUT_PROTO>();
    }
  }

  void Add(PipeDataType pipeElement)
  {
    pipe_.push(std::move(pipeElement));
  }

  StateResult progress()
  {
    if (!last_output || !protoPipeBuilder)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "No last output or pipe builder set");
      wake();
      return StateResult(0, ERRORED);
    }

    if (pipe_.size() == 0)
    {
      if (messageHandler)
      {
        messageHandler(GetOutput());
      }
      wake();
      return StateResult(0, COMPLETE);
    }

    auto                           this_sp = this->template shared_from_base<TaskChainSerial>();
    std::weak_ptr<TaskChainSerial> this_wp = this_sp;

    auto data  = pipe_.front();
    auto input = protoPipeBuilder(last_output, data);

    auto task = CreateTask(data, input);

    if (task == nullptr)
    {
      FETCH_LOG_ERROR(LOGGING_NAME, "Failed to create task!");
      wake();
      return StateResult(0, ERRORED);
    }

    task->SetMessageHandler([this_wp](std::shared_ptr<OUT_PROTO> response) {
      auto sp = this_wp.lock();
      if (sp)
      {
        if (sp->taskResultUpdate)
        {
          sp->last_output = sp->taskResultUpdate(sp, std::move(response));
        }
        else
        {
          sp->last_output = std::move(response);
        }
      }
      else
      {
        FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to TaskChainSerial");
      }
    });

    task->SetErrorHandler(
        [this_wp](const std::string &dap_name, const std::string &path, const std::string &msg) {
          auto sp = this_wp.lock();
          if (sp)
          {
            std::queue<PipeDataType> empty;
            std::swap(sp->pipe_, empty);
            sp->last_output = nullptr;
            if (sp->errorHandler)
            {
              sp->errorHandler(dap_name, path, msg);
              sp->wake();
            }
          }
          else
          {
            FETCH_LOG_ERROR(LOGGING_NAME, "No shared pointer to TaskChainSerial");
          }
        });

    task->submit();

    pipe_.pop();

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
      FETCH_LOG_INFO(LOGGING_NAME, "Sleeping");
      return StateResult(1, DEFER);
    }

    FETCH_LOG_INFO(LOGGING_NAME, "NOT Sleeping");
    return StateResult(1, COMPLETE);
  }

  std::shared_ptr<OUT_PROTO> GetOutput()
  {
    return last_output;
  }

  PipeDataType &GetTopPipeData()
  {
    return pipe_.front();
  }

public:
  ErrorHandler     errorHandler;
  MessageHandler   messageHandler;
  TaskResultUpdate taskResultUpdate;

protected:
  ProtoPipeBuilder protoPipeBuilder;

  std::shared_ptr<OUT_PROTO> last_output = nullptr;
  std::queue<PipeDataType>   pipe_{};

  std::vector<EntryPoint> entryPoint{};
};
