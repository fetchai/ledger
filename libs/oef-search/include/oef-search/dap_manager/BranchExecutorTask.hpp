#pragma once

#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-messages/dap_interface.hpp"
#include "oef-search/dap_manager/Branch.hpp"
#include "oef-search/dap_manager/NodeExecutorTask.hpp"

class BranchExecutorTask : virtual public NodeExecutorTask
{
public:
  static constexpr char const *LOGGING_NAME = "BranchExecutorTask";
  using NodeExecutorTask::SetErrorHandler;
  using NodeExecutorTask::SetMessageHandler;
  using Parent = NodeExecutorTask;

  struct NodeDataType
  {
    std::string                         type;
    std::shared_ptr<Leaf>               leaf   = nullptr;
    std::shared_ptr<Branch>             branch = nullptr;
    std::shared_ptr<IdentifierSequence> prev   = nullptr;

    explicit NodeDataType(std::shared_ptr<Branch> branch_ptr)
     : type{"branch"}
     , branch{std::move(branch_ptr)}
    {}

    explicit NodeDataType(std::shared_ptr<Leaf> leaf_ptr)
     : type{"leaf"}
     , leaf{std::move(leaf_ptr)}
    {}
  };

  BranchExecutorTask(std::shared_ptr<Branch> root)
    : Parent()
    , root_{std::move(root)}
  {}

  virtual ~BranchExecutorTask() = default;

  BranchExecutorTask(const BranchExecutorTask &other) = delete;

  BranchExecutorTask &operator=(const BranchExecutorTask &other) = delete;

  bool operator==(const BranchExecutorTask &other) = delete;

  bool operator<(const BranchExecutorTask &other) = delete;

protected:
protected:
  std::shared_ptr<Branch> root_;
};
