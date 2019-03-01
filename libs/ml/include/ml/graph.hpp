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

#include "ml/node.hpp"
#include "ml/ops/weights.hpp"
#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>

namespace fetch {
namespace ml {

template <class T>
class Graph : public ops::Trainable<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using Datatype     = typename ArrayType::Type;

  Graph()
  {}

  ArrayPtrType Evaluate(std::string const &nodeName)
  {
    return nodes_[nodeName]->Evaluate();
  }

  void BackPropagate(std::string const &nodeName, ArrayPtrType errorSignal)
  {
    nodes_[nodeName]->BackPropagate(errorSignal);
  }

  /*
   * Called for node without trainable parameters
   */
  template <class OperationType, typename... Params>
  typename std::enable_if<!std::is_base_of<ops::Trainable<T>, OperationType>::value>::type AddNode(
      std::string const &nodeName, std::vector<std::string> const &inputs, Params... params)
  {
    nodes_[nodeName] = std::make_shared<Node<ArrayType, OperationType>>(nodeName, params...);
    FETCH_LOG_INFO("ML_LIB", "Creating node [", nodeName, "]");
    for (auto const &i : inputs)
    {
      nodes_[nodeName]->AddInput(nodes_[i]);
    }
  }

  /*
   * Called for nodes with trainable parameters
   * Will keep the node in the trainable_ list to step through them
   */
  template <class OperationType, typename... Params>
  typename std::enable_if<std::is_base_of<ops::Trainable<T>, OperationType>::value>::type AddNode(
      std::string const &nodeName, std::vector<std::string> const &inputs, Params... params)
  {
    std::shared_ptr<Node<ArrayType, OperationType>> op =
        std::make_shared<Node<ArrayType, OperationType>>(nodeName, params...);
    nodes_[nodeName]     = op;
    trainable_[nodeName] = op;
    FETCH_LOG_INFO("ML_LIB", "Creating node [", nodeName, "] -- Register as Trainable");
    for (auto const &i : inputs)
    {
      nodes_[nodeName]->AddInput(nodes_[i]);
    }
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
  virtual void LoadStateDict(struct ops::StateDict<T> const &dict)
  {
    assert(!dict.weights_);
    for (auto const &t : trainable_)
    {
      t.second->LoadStateDict(dict.dict_.at(t.first));
    }
  }

protected:
  std::unordered_map<std::string, std::shared_ptr<fetch::ml::NodeInterface<ArrayType>>>  nodes_;
  std::unordered_map<std::string, std::shared_ptr<fetch::ml::ops::Trainable<ArrayType>>> trainable_;
};

}  // namespace ml
}  // namespace fetch
