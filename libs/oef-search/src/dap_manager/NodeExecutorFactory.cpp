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

#include "oef-base/proto_comms/ProtoMessageSender.hpp"
#include "oef-search/dap_manager/BranchParallelExecutorTask.hpp"
#include "oef-search/dap_manager/BranchSerialExecutorTask.hpp"
#include "oef-search/dap_manager/DapManager.hpp"
#include "oef-search/dap_manager/MementoExecutorTask.hpp"
#include "oef-search/dap_manager/NodeExecutorFactory.hpp"
#include "oef-search/dap_manager/WithLateDapExecutorTask.hpp"
#include <unordered_set>

constexpr char const *LOGGING_NAME = "NodeExecutorFactory";

template <class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
  seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct IdentifierHash
{
  std::size_t operator()(Identifier const &id) const noexcept
  {
    std::size_t seed = std::hash<std::string>{}(id.agent());
    hash_combine(seed, id.core());
    hash_combine(seed, id.score());
    hash_combine(seed, id.uri());
    return seed;
  }
};

inline bool operator==(Identifier const &lhs, Identifier const &rhs)
{
  return (lhs.agent() == rhs.agent()) && (lhs.core() == rhs.core()) && (lhs.uri() == rhs.uri()) &&
         (lhs.score() == rhs.score());
}

std::shared_ptr<NodeExecutorTask> NodeExecutorFactory(const BranchExecutorTask::NodeDataType &data,
                                                      std::shared_ptr<IdentifierSequence>     input,
                                                      std::shared_ptr<DapManager> &dap_manager)
{
  std::shared_ptr<Node> node = nullptr;
  if (data.type == "leaf" && data.leaf)
  {
    node = data.leaf;
  }
  else if (data.type == "branch" && data.branch)
  {
    node = data.branch;
  }
  else
  {
    return nullptr;
  }

  auto &                        dap_store = dap_manager->GetDapStore();
  std::vector<Node::DapMemento> mementos{};
  std::vector<Node::DapMemento> late_mementos{};
  try
  {
    for (auto &mem : node->GetMementos())
    {
      if (dap_store->IsDapEarly(mem.first))
      {
        mementos.push_back(mem);
      }
      else if (dap_store->IsDapLate(mem.first))
      {
        late_mementos.push_back(mem);
      }
      else
      {
        mementos.push_back(mem);
      }
    }
    std::shared_ptr<NodeExecutorTask> task = nullptr;
    if (!mementos.empty())
    {
      task = std::make_shared<MementoExecutorTask>(std::move(mementos), input, dap_manager);
    }
    else if (data.type == "leaf" && data.leaf)
    {
      task = std::dynamic_pointer_cast<NodeExecutorTask>(
          std::make_shared<MementoExecutorTask>(data.leaf->GetMementos(), input, dap_manager));
    }
    else if (data.type == "branch" && data.branch)
    {
      auto op = data.branch->GetOperator();
      if (op == "all" || op == "and" || op == "result")
      {
        task = std::dynamic_pointer_cast<NodeExecutorTask>(
            std::make_shared<BranchSerialExecutorTask>(data.branch, input, dap_manager));
      }
      else if (op == "any" || op == "or")
      {
        task = std::dynamic_pointer_cast<NodeExecutorTask>(
            std::make_shared<BranchParallelExecutorTask>(data.branch, input, dap_manager));
      }
      else if (op == "none")
      {
        if (input->originator())
        {
          FETCH_LOG_WARN(
              LOGGING_NAME, "Now at: ", node->ToString(),
              ". "
              "Trying to create not executor branch with originator IdentifierSequence: ",
              input->DebugString());
          return nullptr;
        }
        auto executor = std::make_shared<BranchSerialExecutorTask>(data.branch, input, dap_manager);
        executor->taskResultUpdate =
            [](std::shared_ptr<BranchSerialExecutorTask::BaseTask> sp,
               std::shared_ptr<IdentifierSequence> result) -> std::shared_ptr<IdentifierSequence> {
          sp->GetTopPipeData().prev = sp->GetOutput();
          return result;
        };
        executor->SetPipeBuilder([](std::shared_ptr<IdentifierSequence>     result,
                                    const BranchExecutorTask::NodeDataType &data)
                                     -> std::shared_ptr<IdentifierSequence> {
          if (data.prev == nullptr)
          {
            FETCH_LOG_ERROR(LOGGING_NAME, "NodeDataType.prev not set! Not execution failed!");
            return result;
          }
          std::unordered_set<Identifier, IdentifierHash> to_remove;
          for (auto &id : result->identifiers())
          {
            to_remove.insert(id);
          }
          auto res = std::make_shared<IdentifierSequence>();
          res->set_originator(result->originator());
          res->mutable_status()->CopyFrom(result->status());
          for (auto &id : data.prev->identifiers())
          {
            auto it = to_remove.find(id);
            if (it == to_remove.end())
            {
              res->add_identifiers()->CopyFrom(id);
            }
          }
          return res;
        });
        task = std::dynamic_pointer_cast<NodeExecutorTask>(executor);
      }
    }
    else
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Failed to create task, because type ", data.type,
                     " not supported, or data not set!");
    }
    if (task == nullptr)
    {
      FETCH_LOG_WARN(LOGGING_NAME, "Now at: ", node->ToString(), "... But task is nullptr!");
      return nullptr;
    }
    FETCH_LOG_INFO(LOGGING_NAME, "Now at: ", node->ToString(), ". Task id=", task->GetTaskId());
    if (!late_mementos.empty())
    {
      FETCH_LOG_INFO(LOGGING_NAME, node->ToString(),
                     " has late daps. Create WithLateDapExecutorTask wrapper task");
      return std::make_shared<WithLateDapExecutorTask>(std::move(task), std::move(late_mementos),
                                                       dap_manager);
    }
    return task;
  }
  catch (std::exception const &e)
  {
    FETCH_LOG_WARN(LOGGING_NAME, "Exception in task creation: ", e.what());
    return nullptr;
  }
}
