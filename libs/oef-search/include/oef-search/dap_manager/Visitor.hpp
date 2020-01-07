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

#include "Branch.hpp"
#include "logging/logging.hpp"
#include "oef-base/threading/ExitState.hpp"
#include "oef-base/threading/Task.hpp"
#include "oef-base/threading/Waitable.hpp"
#include <atomic>
#include <queue>
#include <stack>
#include <utility>

/**
 * BottomUp visiting: bottom to top, use std::stack as template argument
 * VisitDescending: top to bottom, use Queue as template argument
 * @tparam Container
 */
template <template <typename> class Container>
class Visitor : public fetch::oef::base::Task, public fetch::oef::base::Waitable
{
public:
  static constexpr char const *LOGGING_NAME = "Visitor";

  using TreeContainer = Container<std::pair<uint32_t, std::shared_ptr<Node>>>;
  // using StackBuilderAlgorithm = std::function<void(NodeStack&, Branch&, uint32_t)>;

  enum VisitNodeExitStates
  {
    DEFER,
    COMPLETE,
    ERRORED,
    RERUN,
    STOP
  };

  Visitor()
    : runnable_{false}
  {}

  ~Visitor() override           = default;
  Visitor(const Visitor &other) = delete;
  Visitor &operator=(const Visitor &other) = delete;

  bool operator==(const Visitor &other) = delete;
  bool operator<(const Visitor &other)  = delete;

  virtual VisitNodeExitStates VisitNode(Branch &node, uint32_t depth) = 0;
  virtual VisitNodeExitStates VisitLeaf(Leaf &leaf, uint32_t depth)   = 0;

  bool IsRunnable() const override
  {
    return runnable_.load();
  }

  fetch::oef::base::ExitState run() override
  {
    if (tree_.empty())
    {
      wake();
      return fetch::oef::base::ExitState ::COMPLETE;
    }

    while (!tree_.empty())
    {
      auto &              node      = tree_.top();
      auto                node_type = node.second->GetNodeType();
      VisitNodeExitStates state;
      if (node_type == "leaf")
      {
        state = this->VisitLeaf(*std::static_pointer_cast<Leaf>(node.second), node.first);
      }
      else if (node_type == "branch")
      {
        state = this->VisitNode(*std::static_pointer_cast<Branch>(node.second), node.first);
      }
      else
      {
        FETCH_LOG_WARN(LOGGING_NAME, "Unknown node type: ", node_type);
        wake();
        state = VisitNodeExitStates ::ERRORED;
      }

      switch (state)
      {
      case COMPLETE:
      {
        tree_.pop();
        break;
      }
      case DEFER:
      {
        return fetch::oef::base::ExitState ::DEFER;
      }
      case ERRORED:
      {
        return fetch::oef::base::ExitState ::ERRORED;
      }
      case RERUN:
      {
        return fetch::oef::base::ExitState ::RERUN;
      }
      case STOP:
      {
        while (!tree_.empty())
        {
          tree_.pop();
        }
      }
      }
    }
    wake();
    return fetch::oef::base::ExitState ::COMPLETE;
  }

  virtual void SubmitVisitTask(std::shared_ptr<Branch> const &root)
  {
    runnable_.store(true);

    tree_ = TreeContainer();
    TreeBuilder(root, 0);

    this->submit();
  }

protected:
  void TreeBuilder(std::shared_ptr<Branch> const &root, uint32_t depth = 0)
  {
    tree_.push(std::make_pair(depth, root));
    for (auto &leaf : root->GetLeaves())
    {
      tree_.push(std::make_pair(depth + 1, leaf));
    }
    for (auto &node : root->GetSubnodes())
    {
      TreeBuilder(node, depth + 1);
    }
  }

protected:
  std::atomic<bool> runnable_;
  TreeContainer     tree_;

private:
};

template <class T>
class Queue
{
public:
  void push(const T &value)
  {
    queue_.push(value);
  }

  void push(T &&value)
  {
    queue_.push(std::move(value));
  }

  T &top()
  {
    return queue_.front();
  }

  const T &top() const
  {
    return queue_.front();
  }

  void pop()
  {
    queue_.pop();
  }

  bool empty() const
  {
    return queue_.empty();
  }

private:
  std::queue<T> queue_;
};

template <class T>
class Stack
{
public:
  void push(const T &value)
  {
    stack_.push(value);
  }

  void push(T &&value)
  {
    stack_.push(std::move(value));
  }

  T &top()
  {
    return stack_.top();
  }

  const T &top() const
  {
    return stack_.top();
  }

  void pop()
  {
    stack_.pop();
  }

  bool empty() const
  {
    return stack_.empty();
  }

private:
  std::stack<T> stack_;
};
