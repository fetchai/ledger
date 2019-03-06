#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "ml/meta/ml_type_traits.hpp"

#include "ml/node.hpp"
#include "ml/ops/weights.hpp"

#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>

namespace fetch {
namespace ml {

/**
 * The full graph on which to run the computation
 */
template <class T>
class Graph : public ops::Trainable<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using Datatype     = typename ArrayType::Type;

  Graph()
  {}

  ArrayPtrType Evaluate(std::string const &node_name)
  {
    if (nodes_[node_name])
    {
      return nodes_[node_name]->Evaluate();
    }
    else
    {
      throw std::runtime_error(
          std::string("Cannot evaluate: node [" + node_name + "] not in graph"));
    }
  }

  void BackPropagate(std::string const &nodeName, ArrayPtrType errorSignal)
  {
    nodes_[nodeName]->BackPropagate(errorSignal);
  }

  /*
   * Called for node without trainable parameters
   */
  template <class OperationType, typename... Params>
  meta::IfIsNotTrainable<ArrayType, OperationType, std::string> AddNode(
      std::string const &node_name, std::vector<std::string> const &inputs, Params... params)
  {
    std::string name = UpdateVariableName<OperationType>(node_name);
    std::shared_ptr<Node<ArrayType, OperationType>> op =
        std::make_shared<Node<ArrayType, OperationType>>(node_name, params...);
    AddNodeImpl<OperationType>(name, inputs, op, true);
    return name;
  }

  /*
   * Called for nodes with trainable parameters
   * Will keep the node in the trainable_ list to step through them
   */
  template <class OperationType, typename... Params>
  meta::IfIsTrainable<ArrayType, OperationType, std::string> AddNode(
      std::string const &node_name, std::vector<std::string> const &inputs, Params... params)
  {
    std::string name = UpdateVariableName<OperationType>(node_name);
    std::shared_ptr<Node<ArrayType, OperationType>> op =
        std::make_shared<Node<ArrayType, OperationType>>(node_name, params...);
    AddNodeImpl<OperationType>(name, inputs, op, true);
    trainable_[node_name] = op;
    return name;
  }

  void SetInput(std::string const &nodeName, ArrayPtrType data)
  {
    std::shared_ptr<fetch::ml::ops::PlaceHolder<ArrayType>> placeholder =
        std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<ArrayType>>(nodes_[nodeName]);

    if (placeholder)
    {
      placeholder->SetData(data);
      ResetGraphCache();
    }
    else
    {
      std::cerr << "No placeholder node with name [" << nodeName << "]" << std::endl;
      assert(false);
    }
    placeholder->SetData(data);
    ResetGraphCache();
  }

  virtual void Step(Datatype learningRate)
  {
    for (auto &t : trainable_)
    {
      t.second->Step(learningRate);
    }
  }

  void ResetGraphCache()
  {
    for (auto &node : nodes_)
    {
      node.second->ResetCache();
    }
  }

  // Returns the graph trainable parameters as a nested structure for serializing
  virtual struct ops::StateDict<ArrayType> StateDict() const
  {
    struct ops::StateDict<ArrayType> d;
    for (auto const &t : trainable_)
    {
      d.dict_.emplace(t.first, t.second->StateDict());
    }
    return d;
  }

  // Import trainable parameters from an exported model
  virtual void
  LoadStateDict(struct ops::StateDict<T> const &dict)
  {
    assert(!dict.weights_);
    for (auto const &t : trainable_)
    {
      t.second->LoadStateDict(dict.dict_.at(t.first));
    }
  }

private:
  template <typename OperationType, typename... Params>
  void AddNodeImpl(std::string const &node_name, std::vector<std::string> const &inputs,
                   std::shared_ptr<Node<ArrayType, OperationType>> op, bool trainable)
  {
    if (!(nodes_.find(node_name) == nodes_.end()))
    {
      throw std::runtime_error("node named [" + node_name + "] already exists");
    }

    nodes_[node_name] = op;

    FETCH_LOG_INFO("ML_LIB", "Creating node [", node_name, "], trainable: ", trainable);
    for (auto const &i : inputs)
    {
      nodes_[node_name]->AddInput(nodes_[i]);
    }
  }

  /**
   * generates a new variable name if necessary to ensure uniqueness within graph
   * @param pre_string
   * @return
   */
  template <typename OperationType>
  std::string UpdateVariableName(std::string const &name)
  {
    std::string ret           = name;
    std::string op_descriptor = (OperationType::DESCRIPTOR);
    // search graph for existing variable names
    if (ret.empty())
    {
      std::uint64_t name_idx = 0;
      ret                    = op_descriptor + "_" + std::to_string(name_idx);
      while (!(nodes_.find(ret) == nodes_.end()))
      {
        ++name_idx;
        ret = op_descriptor + "_" + std::to_string(name_idx);
      }
    }

    return ret;
  }

protected:
  std::unordered_map<std::string, std::shared_ptr<fetch::ml::NodeInterface<ArrayType>>>  nodes_;
  std::unordered_map<std::string, std::shared_ptr<fetch::ml::ops::Trainable<ArrayType>>> trainable_;
};

}  // namespace ml
}  // namespace fetch
