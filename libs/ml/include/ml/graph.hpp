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

  /**
   * Evaluates the output of a node (calling all necessary forward prop)
   * @param nodeName name of node to evaluate for output
   * @return pointer to array containing node output
   */
  ArrayType const &Evaluate(std::string const &node_name)
  {
    if (nodes_[node_name])
    {
      return nodes_[node_name]->Evaluate();
    }
    else
    {
      throw std::runtime_error("Cannot evaluate: node [" + node_name + "] not in graph");
    }
  }

  /**
   * Backpropagate an error signal through the graph
   * @param nodeName name of node from which to begin backprop
   * @param errorSignal pointer to array containing error signal to backprop
   */
  void BackPropagate(std::string const &nodeName, ArrayType const &errorSignal)
  {
    nodes_[nodeName]->BackPropagate(errorSignal);
  }

  /**
   * Adds a node without trainable parameters.
   * @tparam OperationType Op template type
   * @tparam Params template for input parameters to node
   * @param node_name non-unique specified node name
   * @param inputs names of node inputs to the node
   * @param params input parameters to the node op
   */
  template <class OperationType, typename... Params>
  meta::IfIsNotTrainable<ArrayType, OperationType, std::string> AddNode(
      std::string const &node_name, std::vector<std::string> const &inputs, Params... params)
  {
    std::string name = UpdateVariableName<OperationType>(node_name);
    std::shared_ptr<Node<ArrayType, OperationType>> op =
        std::make_shared<Node<ArrayType, OperationType>>(node_name, params...);
    AddNodeImpl<OperationType>(name, inputs, op);
    FETCH_LOG_INFO("ML_LIB", "Created non-trainable node [", node_name, "]");
    return name;
  }

  /**
   * Adds a node with trainable parameters.
   * @tparam OperationType Op template type
   * @tparam Params template for input parameters to node
   * @param node_name non-unique specified node name
   * @param inputs names of node inputs to the node
   * @param params input parameters to the node op
   */
  template <class OperationType, typename... Params>
  meta::IfIsTrainable<ArrayType, OperationType, std::string> AddNode(
      std::string const &node_name, std::vector<std::string> const &inputs, Params... params)
  {
    std::string name = UpdateVariableName<OperationType>(node_name);
    std::shared_ptr<Node<ArrayType, OperationType>> op =
        std::make_shared<Node<ArrayType, OperationType>>(node_name, params...);
    AddNodeImpl<OperationType>(name, inputs, op);
    FETCH_LOG_INFO("ML_LIB", "Created trainable node [", node_name, "]");
    trainable_[node_name] = op;
    return name;
  }

  /**
   * Assigns data to a placeholder if the node can be found in the graph.
   * Also resets the graph cache to avoid erroneous leftover outputs
   * @param nodeName name of the placeholder node in the graph (must be unique)
   * @param data the pointer to a tensor to assign to the placeholder
   */
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
      throw std::runtime_error("No placeholder node with name [" + nodeName + "] found in graph!");
    }
  }

  /**
   * takes a training step
   * @param learningRate the learning rate (alpha) hyperparameter
   */
  virtual void Step(Datatype learningRate)
  {
    for (auto &t : trainable_)
    {
      t.second->Step(learningRate);
    }
  }

  /**
   * Resets graph cache, clearing stored evaluation outputs
   */
  void ResetGraphCache()
  {
    for (auto &node : nodes_)
    {
      node.second->ResetCache();
    }
  }

  /**
   * Assigns all trainable parameters to a stateDict for exporting and serialising
   * @return  d is the StateDict of all trainable params
   */
  virtual struct ops::StateDict<ArrayType> StateDict() const
  {
    struct ops::StateDict<ArrayType> d;
    for (auto const &t : trainable_)
    {
      d.dict_.emplace(t.first, t.second->StateDict());
    }
    return d;
  }

  //

  /**
   * Import trainable parameters from an exported model
   * @param dict  state dictionary to import to weights
   */
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
  /**
   * Implementation of AddNode that is common to trainable and non-trainable ops
   * @tparam OperationType  The type of Op (or layer) to be added
   * @param node_name  the initial provided node name - might not be unique
   * @param inputs  vector of names of nodes that input to this node
   * @param op  the op to be added to the graph as a new node
   */
  template <typename OperationType>
  void AddNodeImpl(std::string const &node_name, std::vector<std::string> const &inputs,
                   std::shared_ptr<Node<ArrayType, OperationType>> op)
  {
    if (!(nodes_.find(node_name) == nodes_.end()))
    {
      throw std::runtime_error("node named [" + node_name + "] already exists");
    }

    nodes_[node_name] = op;

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
