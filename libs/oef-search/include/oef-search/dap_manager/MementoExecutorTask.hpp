#pragma once

#include "DapManager.hpp"
#include "Leaf.hpp"
#include "NodeExecutorTask.hpp"
#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/Future.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-messages/dap_interface.hpp"
#include "oef-search/dap_comms/DapSerialConversationTask.hpp"

// DapExecute, IdentifierSequence
class MementoExecutorTask : virtual public NodeExecutorTask
{
public:
  static constexpr char const *LOGGING_NAME = "LeafTask";

  using NodeExecutorTask::ErrorHandler;
  using NodeExecutorTask::MessageHandler;
  using ConvTask =
      DapSerialConversationTask<DapExecute, IdentifierSequence, ConstructQueryMementoResponse>;

  MementoExecutorTask(std::vector<Node::DapMemento>       mementos,
                      std::shared_ptr<IdentifierSequence> identifier_sequence,
                      std::shared_ptr<DapManager>         dap_manager)
    : NodeExecutorTask()
    , mementos_{std::move(mementos)}
    , dap_manager_{std::move(dap_manager)}
    , conv_task_{nullptr}
    , identifier_sequence_{std::move(identifier_sequence)}
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task created, id=", this->GetTaskId());
  }

  virtual ~MementoExecutorTask()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone, id=", this->GetTaskId());
  }

  MementoExecutorTask(const MementoExecutorTask &other) = delete;
  MementoExecutorTask &operator=(const MementoExecutorTask &other) = delete;

  bool operator==(const MementoExecutorTask &other) = delete;
  bool operator<(const MementoExecutorTask &other)  = delete;

  virtual bool IsRunnable() const override
  {
    return true;
  }

  virtual ExitState run(void) override
  {
    if (conv_task_ == nullptr)
    {
      conv_task_ = std::make_shared<ConvTask>(dap_manager_->GetNewSerialCallId(),
                                              dap_manager_->GetOutbounds());
      conv_task_->InitPipe(identifier_sequence_);

      for (auto &mem_pair : mementos_)
      {
        conv_task_->Add(DapInputDataType<ConstructQueryMementoResponse>{mem_pair.first, "execute",
                                                                        mem_pair.second});
      }

      conv_task_->SetPipeBuilder([](std::shared_ptr<IdentifierSequence>                    ids,
                                    DapInputDataType<ConstructQueryMementoResponse> const &data)
                                     -> std::shared_ptr<DapExecute> {
        auto in_proto = std::make_shared<DapExecute>();
        in_proto->mutable_query_memento()->CopyFrom(*(data.proto));
        in_proto->mutable_input_idents()->CopyFrom(*ids);
        return in_proto;
      });

      auto this_sp = this->template shared_from_base<MementoExecutorTask>();
      std::weak_ptr<MementoExecutorTask> this_wp = this_sp;
      auto id = this->GetTaskId();
      auto task_id = conv_task_->GetTaskId();

      conv_task_->errorHandler = [this_wp, id, task_id](const std::string &dap_name, const std::string &path,
                                           const std::string &msg) {
        auto sp = this_wp.lock();
        if (sp && sp->errorHandler)
        {
          sp->errorHandler(dap_name, path, msg);
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "id=", id, ", task_id=", task_id, ";Failed to execute memento chain, because call to dap ",
                         dap_name, " failed! Message: ", msg);
        }
        if (sp)
        {
          sp->wake();
        }
      };

      conv_task_->messageHandler = [this_wp, id, task_id](std::shared_ptr<IdentifierSequence> result) {
        auto sp = this_wp.lock();
        if (sp)
        {
          if (sp->messageHandler)
          {
            sp->messageHandler(std::move(result));
          }
          else
          {
            FETCH_LOG_WARN(LOGGING_NAME, "id=", id, ", task_id=", task_id, "; No messageHandler, loosing output!");
          }
          sp->wake();
        }
        else
        {
          FETCH_LOG_WARN(LOGGING_NAME, "id=", id, ", task_id=", task_id, "; Failed to set result, no shared_ptr!");
        }
      };

      conv_task_->submit();

      if (conv_task_->MakeNotification()
              .Then([this_wp]() {
                auto sp = this_wp.lock();
                if (sp)
                {
                  sp->MakeRunnable();
                }
              })
              .Waiting())
      {
        FETCH_LOG_INFO(LOGGING_NAME, "Sleeping (id=", this->GetTaskId(), "), will be woken by conv task ", conv_task_->GetTaskId());
        return ExitState ::DEFER;
      }
    }
    FETCH_LOG_INFO(LOGGING_NAME, "NOT Sleeping");

    return ExitState ::COMPLETE;
  }

  virtual void SetMessageHandler(MessageHandler mH) override
  {
    messageHandler = std::move(mH);
  }

  virtual void SetErrorHandler(ErrorHandler eH) override
  {
    errorHandler = std::move(eH);
  }

public:
  MessageHandler messageHandler;
  ErrorHandler   errorHandler;

protected:
  std::vector<Node::DapMemento>       mementos_;
  std::shared_ptr<DapManager>         dap_manager_;
  std::shared_ptr<ConvTask>           conv_task_;
  std::shared_ptr<IdentifierSequence> identifier_sequence_;
};
