#pragma once

#include "BranchExecutorTask.hpp"
#include "BranchParallelExecutorTask.hpp"
#include "DapManager.hpp"
#include "MementoExecutorTask.hpp"
#include "NodeExecutorFactory.hpp"
#include "oef-base/threading/TaskChainSerial.hpp"

class BranchSerialExecutorTask
  : virtual public BranchExecutorTask,
    virtual public TaskChainSerial<IdentifierSequence, IdentifierSequence,
                                   BranchExecutorTask::NodeDataType, NodeExecutorTask>
{
public:
  using BaseTask       = TaskChainSerial<IdentifierSequence, IdentifierSequence,
                                   BranchExecutorTask::NodeDataType, NodeExecutorTask>;
  using MessageHandler = BaseTask ::MessageHandler;
  using ErrorHandler   = BaseTask ::ErrorHandler;

  using BaseTask::SetPipeBuilder;
  using BaseTask::taskResultUpdate;

  static constexpr char const *LOGGING_NAME = "BranchSerialExecutorTask";

  BranchSerialExecutorTask(std::shared_ptr<Branch>             root,
                           std::shared_ptr<IdentifierSequence> identifier_sequence,
                           std::shared_ptr<DapManager>         dap_manager)
    : BranchExecutorTask::Parent()
    , BranchExecutorTask(std::move(root))
    , BaseTask ::Parent()
    , BaseTask()
    , dap_manager_{std::move(dap_manager)}
  {
    for (auto &leaf : root_->GetLeaves())
    {
      this->Add(NodeDataType(leaf));
    }

    for (auto &branch : root_->GetSubnodes())
    {
      this->Add(NodeDataType(branch));
    }

    this->SetPipeBuilder(
        [](std::shared_ptr<IdentifierSequence> input,
           const BranchExecutorTask::NodeDataType &) -> std::shared_ptr<IdentifierSequence> {
          return input;
        });

    this->InitPipe(identifier_sequence);
  }
  virtual ~BranchSerialExecutorTask()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone, id=", this->GetTaskId());
  }

  BranchSerialExecutorTask(const BranchSerialExecutorTask &other) = delete;
  BranchSerialExecutorTask &operator=(const BranchSerialExecutorTask &other) = delete;

  bool operator==(const BranchSerialExecutorTask &other) = delete;
  bool operator<(const BranchSerialExecutorTask &other)  = delete;

  virtual std::shared_ptr<NodeExecutorTask> CreateTask(
      const BranchExecutorTask::NodeDataType &data,
      std::shared_ptr<IdentifierSequence>     input) override
  {
    return NodeExecutorFactory(data, input, dap_manager_);
  }

  virtual void SetMessageHandler(MessageHandler mH) override
  {
    this->messageHandler = std::move(mH);
  }

  virtual void SetErrorHandler(ErrorHandler eH) override
  {
    this->errorHandler = std::move(eH);
  }

protected:
  std::shared_ptr<DapManager> dap_manager_;
};
