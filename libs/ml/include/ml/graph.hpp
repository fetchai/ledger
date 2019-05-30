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

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {

/**
 * The full graph on which to run the computation
 */
template <class T>
class Graph : public ops::Trainable<T>
{
public:
  using ArrayType          = T;
  using ArrayPtrType       = std::shared_ptr<ArrayType>;
  using Datatype           = typename ArrayType::Type;
  using ConstSliceType     = typename ArrayType::ConstSliceType;
  using NodePtrType        = typename std::shared_ptr<fetch::ml::NodeInterface<ArrayType>>;
  using TrainablePtrType   = typename std::shared_ptr<fetch::ml::ops::Trainable<ArrayType>>;
  using PlaceholderType    = typename fetch::ml::ops::PlaceHolder<ArrayType>;
  using PlaceholderPtrType = typename std::shared_ptr<fetch::ml::ops::PlaceHolder<ArrayType>>;

  Graph() = default;

  ArrayType    Evaluate(std::string const &node_name);
  void         BackPropagate(std::string const &node_name, ArrayType const &error_signal);
  virtual void Step(Datatype learningRate);

  template <class OperationType, typename... Params>
  std::string AddNode(std::string const &node_name, std::vector<std::string> const &inputs,
                      Params... params);
  NodePtrType GetNode(std::string const &node_name) const;
  void        SetInput(std::string const &node_name, ArrayType data, bool batch = false);
  void        ResetGraphCache(std::shared_ptr<NodeInterface<T>> const &n, bool input_size_changed);

  virtual struct fetch::ml::StateDict<ArrayType> StateDict() const;
  virtual void LoadStateDict(struct fetch::ml::StateDict<T> const &dict);

private:
  /**
   * Appends op to map of trainable nodes. Called by AddNode if the node is for a trainable op
   * @tparam OperationType template class of operation
   * @param name the guaranteed unique name of the node
   * @param op the pointer to the op
   */
  template <class OperationType>
  meta::IfIsTrainable<ArrayType, OperationType, void> AddTrainable(
      std::string const &name, std::shared_ptr<Node<ArrayType, OperationType>> op)
  {
    FETCH_LOG_INFO("ML_LIB", "Created trainable node [", name, "]");
    trainable_[name] = op;
  }

  /**
   * If AddNode is called for a non-trainable op, this version of the function is called which
   * does not append to the trainable map
   * @tparam OperationType
   * @param name
   * @param op
   * @return
   */
  template <class OperationType>
  meta::IfIsNotTrainable<ArrayType, OperationType, void> AddTrainable(
      std::string const &name, std::shared_ptr<Node<ArrayType, OperationType>> op)
  {
    FETCH_UNUSED(name);
    FETCH_UNUSED(op);
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
  std::unordered_map<std::string, NodePtrType>      nodes_;
  std::unordered_map<std::string, TrainablePtrType> trainable_;
};

/**
 * Evaluates the output of a node (calling all necessary forward prop)
 * @param node_name name of node to evaluate for output
 * @return pointer to array containing node output
 */
template <typename ArrayType>
ArrayType Graph<ArrayType>::Evaluate(std::string const &node_name)
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
 * @param error_signal pointer to array containing error signal to backprop
 */
template <typename ArrayType>
void Graph<ArrayType>::BackPropagate(std::string const &node_name, ArrayType const &error_signal)
{
  nodes_[node_name]->BackPropagate(error_signal);
}

/**
 * takes a training step
 * @param learningRate the learning rate (alpha) hyperparameter
 */
template <typename ArrayType>
void Graph<ArrayType>::Step(Datatype learningRate)
{
  for (auto &t : trainable_)
  {
    t.second->Step(learningRate);
  }
}

/**
 * Adds a node without trainable parameters.
 * @tparam OperationType Op template type
 * @tparam Params template for input parameters to node
 * @param node_name non-unique specified node name
 * @param inputs names of node inputs to the node
 * @param params input parameters to the node op
 */
template <typename ArrayType>
template <class OperationType, typename... Params>
std::string Graph<ArrayType>::AddNode(std::string const &             node_name,
                                      std::vector<std::string> const &inputs, Params... params)
{
  // guarantee unique op name
  std::string name = UpdateVariableName<OperationType>(node_name);

  // Instantiate the node
  auto op      = std::make_shared<Node<ArrayType, OperationType>>(name, params...);
  nodes_[name] = op;

  // assign inputs and outputs
  for (auto const &i : inputs)
  {
    nodes_[name]->AddInput(nodes_[i]);
    nodes_[i]->AddOutput(nodes_[name]);
  }

  // add to map of trainable ops if necessary
  AddTrainable(name, op);

  // return unique node name (may not be identical to node_name)
  return name;
}

template <typename ArrayType>
typename Graph<ArrayType>::NodePtrType Graph<ArrayType>::GetNode(std::string const &node_name) const
{
  NodePtrType ret = nodes_.at(node_name);
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
 */
template <typename ArrayType>
void Graph<ArrayType>::SetInput(std::string const &node_name, ArrayType data, bool batch)
{
  PlaceholderPtrType placeholder = std::dynamic_pointer_cast<PlaceholderType>(nodes_[node_name]);

  if (placeholder)
  {
    bool input_size_changed = placeholder->SetData(data);
    ResetGraphCache(nodes_[node_name], input_size_changed);
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
 * Resets graph cache, clearing stored evaluation outputs
 * and recursively updating the input size for all downstream nodes
 */
template <typename ArrayType>
void Graph<ArrayType>::ResetGraphCache(NodePtrType const &n, bool input_size_changed)
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
template <typename ArrayType>
struct fetch::ml::StateDict<ArrayType> Graph<ArrayType>::StateDict() const
{
  struct fetch::ml::StateDict<ArrayType> d;
  for (auto const &t : trainable_)
  {
    d.dict_.emplace(t.first, t.second->StateDict());
  }
  return d;
}

/**
 * Import trainable parameters from an exported model
 * @param dict  state dictionary to import to weights
 */
template <typename ArrayType>
void Graph<ArrayType>::LoadStateDict(struct fetch::ml::StateDict<ArrayType> const &dict)
{
  assert(!dict.weights_);
  for (auto const &t : trainable_)
  {
    t.second->LoadStateDict(dict.dict_.at(t.first));
  }
}

}  // namespace ml
}  // namespace fetch
