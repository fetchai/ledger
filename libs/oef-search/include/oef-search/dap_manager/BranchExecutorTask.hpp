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

  explicit BranchExecutorTask(std::shared_ptr<Branch> root)
    : root_{std::move(root)}
  {}

  ~BranchExecutorTask() override = default;

  BranchExecutorTask(const BranchExecutorTask &other) = delete;

  BranchExecutorTask &operator=(const BranchExecutorTask &other) = delete;

  bool operator==(const BranchExecutorTask &other) = delete;

  bool operator<(const BranchExecutorTask &other) = delete;

protected:
protected:
  std::shared_ptr<Branch> root_;
};
