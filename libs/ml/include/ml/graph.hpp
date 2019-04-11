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
   * @param node_name name of node to evaluate for output
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
   * @param node_name name of node from which to begin backprop
   * @param errorSignal pointer to array containing error signal to backprop
   */
  void BackPropagate(std::string const &node_name, ArrayType const &errorSignal)
  {
    nodes_[node_name]->BackPropagate(errorSignal);
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
        std::make_shared<Node<ArrayType, OperationType>>(name, params...);
    AddNodeImpl<OperationType>(name, inputs, op);
    FETCH_LOG_INFO("ML_LIB", "Created non-trainable node [", name, "]");
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
        std::make_shared<Node<ArrayType, OperationType>>(name, params...);
    AddNodeImpl<OperationType>(name, inputs, op);
    FETCH_LOG_INFO("ML_LIB", "Created trainable node [", name, "]");
    trainable_[name] = op;
    return name;
  }

  std::shared_ptr<fetch::ml::NodeInterface<ArrayType>> GetNode(std::string const &node_name) const
  {
    std::shared_ptr<fetch::ml::NodeInterface<ArrayType>> ret = nodes_.at(node_name);
    if (!ret)
    {
      throw std::runtime_error("couldn't find node [" + node_name + "] in graph!");
    }
    return ret;
  }

  /**
   * Assigns data to a placeholder if the node can be found in the graph.
   * Also resets the graph cache to avoid erroneous leftover outputs
   * @param node_name name of the placeholder node in the graph (must be unique)
   * @param data the pointer to a tensor to assign to the placeholder
   * @param batch flag to indicate if input should be treated as batch
   * @param data flag to indicate is the grapg should reallocate it's output buffers
   * This is a workaround to delay computation untill all inputs are available, call SetInput with
   * recompute_cache = false if some Placeholder are still empty
   */
  void SetInput(std::string const &node_name, ArrayType data, bool batch = false,
                bool recompute_cache = true)
  {
    std::shared_ptr<fetch::ml::ops::PlaceHolder<ArrayType>> placeholder =
        std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<ArrayType>>(nodes_[node_name]);

    if (placeholder)
    {
      bool input_size_changed = placeholder->SetData(data);
      ResetGraphCache(nodes_[node_name], input_size_changed & recompute_cache);
    }
    else
    {
      throw std::runtime_error("No placeholder node with name [" + node_name + "] found in graph!");
    }
    for (auto &n : nodes_)
    {
      n.second->SetBatch(batch);
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
   * and recursively updating the input size for all downstream nodes
   */
  void ResetGraphCache(std::shared_ptr<NodeInterface<T>> const &n, bool input_size_changed)
  {
    n->ResetCache(input_size_changed);
    for (auto &node : n->GetOutputs())
    {
      ResetGraphCache(node, input_size_changed);
    }
  }

  /**
   * Assigns all trainable parameters to a stateDict for exporting and serialising
   * @return  d is the StateDict of all trainable params
   */
  virtual struct fetch::ml::StateDict<ArrayType> StateDict() const
  {
    struct fetch::ml::StateDict<ArrayType> d;
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
  LoadStateDict(struct fetch::ml::StateDict<T> const &dict)
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
      nodes_[i]->AddOutput(nodes_[node_name]);
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
