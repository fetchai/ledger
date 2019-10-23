#pragma once

#include "BranchExecutorTask.hpp"
#include "DapManager.hpp"
#include "MementoExecutorTask.hpp"
#include "NodeExecutorFactory.hpp"
#include "oef-base/threading/TaskChainParallel.hpp"

class BranchParallelExecutorTask
  : virtual public BranchExecutorTask,
    virtual public TaskChainParallel<IdentifierSequence, IdentifierSequence,
                                     BranchExecutorTask::NodeDataType, NodeExecutorTask>
{
public:
  using BaseTask       = TaskChainParallel<IdentifierSequence, IdentifierSequence,
                                     BranchExecutorTask::NodeDataType, NodeExecutorTask>;
  using MessageHandler = BaseTask ::MessageHandler;
  using ErrorHandler   = BaseTask ::ErrorHandler;

  static constexpr char const *LOGGING_NAME = "BranchParallelExecutorTask";

  BranchParallelExecutorTask(std::shared_ptr<Branch>             root,
                             std::shared_ptr<IdentifierSequence> identifier_sequence,
                             std::shared_ptr<DapManager>         dap_manager)
    : BranchExecutorTask::Parent()
    , BranchExecutorTask(std::move(root))
    , BaseTask ::Parent()
    , BaseTask()
    , dap_manager_{std::move(dap_manager)}
  {
    this->SetGlobalInput(identifier_sequence);
    this->outputMerger = [](std::vector<std::shared_ptr<IdentifierSequence>> &results) {
      auto merged = std::make_shared<IdentifierSequence>();
      merged->mutable_status()->set_success(true);
      for (auto &res : results)
      {
        for (auto const &id : res->identifiers())
        {
          merged->add_identifiers()->CopyFrom(id);
        }
      }
      return merged;
    };
    for (auto &leaf : root_->GetLeaves())
    {
      this->Add(NodeDataType(leaf));
    }

    for (auto &branch : root_->GetSubnodes())
    {
      this->Add(NodeDataType(branch));
    }
    FETCH_LOG_INFO(LOGGING_NAME, "Task created, id=", this->GetTaskId());
  }

  virtual ~BranchParallelExecutorTask()
  {
    FETCH_LOG_INFO(LOGGING_NAME, "Task gone, id=", this->GetTaskId());
  }

  BranchParallelExecutorTask(const BranchParallelExecutorTask &other) = delete;
  BranchParallelExecutorTask &operator=(const BranchParallelExecutorTask &other) = delete;

  bool operator==(const BranchParallelExecutorTask &other) = delete;
  bool operator<(const BranchParallelExecutorTask &other)  = delete;

  virtual std::shared_ptr<NodeExecutorTask> CreateTask(
      const BranchExecutorTask::NodeDataType &data,
      std::shared_ptr<IdentifierSequence>     input) override
  {
    return NodeExecutorFactory(data, input, dap_manager_);
  }

  virtual std::shared_ptr<IdentifierSequence> GetInputProto(
      const BranchExecutorTask::NodeDataType &) override
  {
    return nullptr;
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
protected:
  std::shared_ptr<DapManager> dap_manager_;
};
