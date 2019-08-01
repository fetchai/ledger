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

#include "ml/core/node.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/op_interface.hpp"
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
class Graph
{
public:
  using ArrayType          = T;
  using ArrayPtrType       = std::shared_ptr<ArrayType>;
  using SizeType           = typename ArrayType::SizeType;
  using DataType           = typename ArrayType::Type;
  using NodePtrType        = typename std::shared_ptr<fetch::ml::Node<ArrayType>>;
  using TrainablePtrType   = typename std::shared_ptr<fetch::ml::ops::Trainable<ArrayType>>;
  using GraphPtrType       = typename std::shared_ptr<fetch::ml::Graph<ArrayType>>;
  using PlaceholderType    = typename fetch::ml::ops::PlaceHolder<ArrayType>;
  using PlaceholderPtrType = typename std::shared_ptr<fetch::ml::ops::PlaceHolder<ArrayType>>;
  using RegPtrType         = std::shared_ptr<fetch::ml::regularisers::Regulariser<T>>;
  using SPType             = GraphSaveableParams<ArrayType>;

  virtual ~Graph() = default;
  Graph()          = default;

  ArrayType    Evaluate(std::string const &node_name, bool is_training = true);
  void         BackPropagateSignal(std::string const &node_name, ArrayType const &error_signal);
  void         BackPropagateError(std::string const &node_name);
  virtual void Step(DataType learning_rate);

  template <class OperationType, typename... Params>
  meta::IfIsShareable<ArrayType, OperationType, std::string> AddNode(
      std::string const &node_name, std::vector<std::string> const &inputs, Params... params);

  template <class OperationType, typename... Params>
  meta::IfIsNotShareable<ArrayType, OperationType, std::string> AddNode(
      std::string const &node_name, std::vector<std::string> const &inputs, Params... params);

  NodePtrType GetNode(std::string const &node_name) const;
  void        SetInput(std::string const &node_name, ArrayType data);
  void        ResetGraphCache(std::shared_ptr<Node<T>> const &n, bool input_size_changed);
  void SetRegularisation(RegPtrType regulariser, DataType regularisation_rate = DataType{0.0});
  bool SetRegularisation(std::string node_name, RegPtrType regulariser,
                         DataType regularisation_rate = DataType{0.0});

  virtual struct fetch::ml::StateDict<ArrayType> StateDict() const;
  virtual void LoadStateDict(struct fetch::ml::StateDict<T> const &dict);

  std::vector<ArrayType> get_weights() const;
  std::vector<ArrayType> GetGradients() const;
  void                   ApplyGradients(std::vector<ArrayType> &grad);

  std::vector<TrainablePtrType>             GetTrainable();
  std::unordered_map<std::string, SizeType> GetTrainableLookup();

  void                           ResetGradients();
  GraphSaveableParams<ArrayType> GetGraphSaveableParams();

  static constexpr char const *DESCRIPTOR = "Graph";

private:
  void ApplyRegularisation();

  template <class OperationType>
  meta::IfIsTrainable<ArrayType, OperationType, void> AddTrainable(
      std::string const &name, std::shared_ptr<Node<ArrayType>> op);

  template <class OperationType>
  meta::IfIsGraph<ArrayType, OperationType, void> AddTrainable(std::string const &name,
                                                               std::shared_ptr<Node<ArrayType>> op);

  template <class OperationType>
  meta::IfIsNotGraphOrTrainable<ArrayType, OperationType, void> AddTrainable(
      std::string const &name, std::shared_ptr<Node<ArrayType>> op);

  template <typename OperationType>
  bool UpdateVariableName(std::string const &name, std::string &ret);

  template <class OperationType>
  void LinkNodesInGraph(std::string const &node_name, std::vector<std::string> const &inputs,
                        std::shared_ptr<fetch::ml::Node<ArrayType>> op);

protected:
  std::unordered_map<std::string, NodePtrType> nodes_;
  std::unordered_map<std::string, SizeType>    trainable_lookup_;
  std::vector<TrainablePtrType>                trainable_;
  std::vector<std::pair<std::string, std::vector<std::string>>>
      connections_;  // unique node name to list of inputs
};

/**
 * Set regularisation type and rate for all trainables in graph
 * @tparam ArrayType
 * @param regularisation_type L1, L2 or NONE
 * @param regularisation_rate
 */
template <typename ArrayType>
void Graph<ArrayType>::SetRegularisation(RegPtrType regulariser, DataType regularisation_rate)
{
  for (auto &t : trainable_)
  {
    t->SetRegularisation(regulariser, regularisation_rate);
  }
}

/**
 * Set regularisation type and rate for specified trainable by it's name
 * @tparam ArrayType
 * @param node_name name of specific trainable
 * @param regularisation_type L1, L2 or NONE
 * @param regularisation_rate
 */
template <typename ArrayType>
bool Graph<ArrayType>::SetRegularisation(std::string node_name, RegPtrType regulariser,
                                         DataType regularisation_rate)
{
  if (nodes_[node_name])
  {
    trainable_[node_name]->SetRegularisation(regulariser, regularisation_rate);
    return true;
  }
  else
  {
    // Trainable doesn't exist
    return false;
  }
}

/**
 * Evaluates the output of a node (calling all necessary forward prop)
 * @param node_name name of node to evaluate for output
 * @return pointer to array containing node output
 */
template <typename ArrayType>
ArrayType Graph<ArrayType>::Evaluate(std::string const &node_name, bool is_training)
{
  if (nodes_[node_name])
  {
    return *(nodes_[node_name]->Evaluate(is_training));
  }
  else
  {
    throw std::runtime_error("Cannot evaluate: node [" + node_name + "] not in graph");
  }
}

/**
 * Backpropagate an error signal through the graph.
 * Given node needs to expect empty error_signal (loss function)
 * @param node_name name of node from which to begin backprop
 */
template <typename ArrayType>
void Graph<ArrayType>::BackPropagateError(std::string const &node_name)
{
  ArrayType error_signal;
  nodes_[node_name]->BackPropagateSignal(error_signal);

  // Applies regularisation to all trainables based on their configuration
  ApplyRegularisation();
}

/**
 * Backpropagate given error signal through the graph
 * @param node_name name of node from which to begin backprop
 * @param error_signal pointer to array containing error signal to backprop
 */
template <typename ArrayType>
void Graph<ArrayType>::BackPropagateSignal(std::string const &node_name,
                                           ArrayType const &  error_signal)
{
  nodes_[node_name]->BackPropagateSignal(error_signal);
}

/**
 * takes a training step
 * @param learning_rate the learning rate (alpha) hyperparameter
 */
template <typename ArrayType>
void Graph<ArrayType>::Step(DataType learning_rate)
{
  for (auto &t : trainable_)
  {
    t->Step(learning_rate);
  }
}

template <typename ArrayType>
void Graph<ArrayType>::ApplyRegularisation()
{
  for (auto &t : trainable_)
  {
    t->ApplyRegularisation();
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
meta::IfIsShareable<ArrayType, OperationType, std::string> Graph<ArrayType>::AddNode(
    std::string const &node_name, std::vector<std::string> const &inputs, Params... params)
{
  // guarantee unique op name
  std::string updated_name;
  bool        is_duplicate = UpdateVariableName<OperationType>(node_name, updated_name);

  std::shared_ptr<fetch::ml::Node<ArrayType>> op;
  if (!is_duplicate)
  {
    // Instantiate the node based on params
    op = std::make_shared<Node<ArrayType>>(OperationType::OpCode(), updated_name, [params...]() {
      return std::make_shared<OperationType>(OperationType(params...));
    });
  }
  else
  {  // if shared weight is specified by duplicate naming
    // Instantiate the node based on pointer to shared target node
    NodePtrType target_node = GetNode(node_name);
    op                      = std::make_shared<Node<ArrayType>>(
        OperationType::OpCode(), updated_name, [target_node, params...]() {
          return std::make_shared<OperationType>(OperationType(params...));
        });
  }

  // assign inputs and outputs to the new node
  LinkNodesInGraph<OperationType>(updated_name, inputs, op);

  // add to map of trainable ops if necessary
  AddTrainable<OperationType>(updated_name, op);

  // return unique node name (may not be identical to node_name)
  return updated_name;
}

template <typename ArrayType>
template <class OperationType, typename... Params>
meta::IfIsNotShareable<ArrayType, OperationType, std::string> Graph<ArrayType>::AddNode(
    std::string const &node_name, std::vector<std::string> const &inputs, Params... params)
{
  // guarantee unique op name
  std::string updated_name;
  UpdateVariableName<OperationType>(node_name, updated_name);

  auto node = Node<ArrayType>(OperationType::OpCode(), updated_name, [params...]() {
    return std::make_shared<OperationType>(OperationType(params...));
  });

  auto op = std::make_shared<Node<ArrayType>>(node);

  // assign inputs and outputs to the new node
  LinkNodesInGraph<OperationType>(updated_name, inputs, op);

  // add to map of trainable ops if necessary
  AddTrainable<OperationType>(updated_name, op);

  // return unique node name (may not be identical to node_name)
  return updated_name;
}

template <typename ArrayType>
GraphSaveableParams<ArrayType> Graph<ArrayType>::GetGraphSaveableParams()
{
  GraphSaveableParams<ArrayType> gs;
  gs.connections = connections_;

  for (auto const &node : nodes_)
  {
    auto nsp = node.second->GetNodeSaveableParams();
    (gs.nodes).insert(std::make_pair(node.first, nsp));
  }
  return gs;
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
 */
template <typename ArrayType>
void Graph<ArrayType>::SetInput(std::string const &node_name, ArrayType data)
{
  PlaceholderPtrType placeholder =
      std::dynamic_pointer_cast<PlaceholderType>(nodes_[node_name]->GetOp());

  if (placeholder)
  {
    bool input_size_changed = placeholder->SetData(data);
    ResetGraphCache(nodes_[node_name], input_size_changed);
  }
  else
  {
    throw std::runtime_error("No placeholder node with name [" + node_name + "] found in graph!");
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
  for (auto const &t : trainable_lookup_)
  {
    d.dict_.emplace(t.first, trainable_.at(t.second)->StateDict());
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
  for (auto const &t : trainable_lookup_)
  {
    trainable_.at(t.second)->LoadStateDict(dict.dict_.at(t.first));
  }
}

/**
 * Assigns all trainable weights parameters to vector of ArrayType for exporting and serialising
 * @return ret is vector containing values for all weights
 */
template <typename ArrayType>
std::vector<ArrayType> Graph<ArrayType>::get_weights() const
{
  std::vector<ArrayType> ret;

  for (auto const &t : trainable_)
  {
    ret.emplace_back(t->get_weights());
  }
  return std::move(ret);
}

/**
 * Assigns all trainable accumulated gradient parameters to vector of ArrayType for exporting and
 * serialising
 * @return ret is vector containing all gradient values
 */
template <typename ArrayType>
std::vector<ArrayType> Graph<ArrayType>::GetGradients() const
{
  std::vector<ArrayType> ret;

  for (auto const &t : trainable_)
  {
    ret.emplace_back(t->get_gradients());
  }
  return std::move(ret);
}

/**
 * Sets all accumulated gradients for each trainable to zero
 */
template <typename ArrayType>
void Graph<ArrayType>::ResetGradients()
{
  for (auto const &t : trainable_)
  {
    t->ResetGradients();
  }
}

/**
 * Add gradient values to weight for each trainable
 * @param grad vector of gradient values for each trainable stored in ArrayType
 */
template <typename ArrayType>
void Graph<ArrayType>::ApplyGradients(std::vector<ArrayType> &grad)
{
  auto grad_it = grad.begin();
  for (auto const &t : trainable_)
  {
    t->ApplyGradient(*grad_it);
    ++grad_it;
  }
}

/**
 * Connect the new node to the current graph by setting input and output nodes to it and saving it
 * in the lookup table
 * @tparam ArrayType
 * @tparam OperationType
 * @tparam Params
 * @param node_name
 * @param inputs
 * @param op
 */
template <typename ArrayType>
template <class OperationType>
void Graph<ArrayType>::LinkNodesInGraph(std::string const &                         node_name,
                                        std::vector<std::string> const &            inputs,
                                        std::shared_ptr<fetch::ml::Node<ArrayType>> op)
{
  // put node in look up table
  nodes_[node_name] = op;

  // assign inputs and outputs
  for (auto const &i : inputs)
  {
    nodes_[node_name]->AddInput(nodes_[i]);
    nodes_[i]->AddOutput(nodes_[node_name]);
  }
}

/**
 * Appends op to map of trainable nodes. Called by AddNode if the node is for a trainable op
 * @tparam OperationType template class of operation
 * @param name the guaranteed unique name of the node
 * @param op the pointer to the op
 */
template <typename ArrayType>
template <class OperationType>
meta::IfIsTrainable<ArrayType, OperationType, void> Graph<ArrayType>::AddTrainable(
    std::string const &name, std::shared_ptr<Node<ArrayType>> node_ptr)
{
  auto node_op = node_ptr->GetOp();
  // it must be safe to cast this op down to a weight
  trainable_.emplace_back(std::dynamic_pointer_cast<ops::Weights<ArrayType>>(node_op));
  trainable_lookup_[name] = trainable_.size() - 1;
}

/**
 * Appends all trainable ops from op.trainable_ to map of trainable nodes.
 * Called by AddNode if the OperationType is a graph
 * @tparam OperationType template class of operation
 * @param name the guaranteed unique name of the node
 * @param op the pointer to the op
 */
template <typename ArrayType>
template <class OperationType>
meta::IfIsGraph<ArrayType, OperationType, void> Graph<ArrayType>::AddTrainable(
    std::string const &name, std::shared_ptr<Node<ArrayType>> op)
{
  auto graph_ptr = std::dynamic_pointer_cast<Graph<ArrayType>>(op->GetOp());
  for (auto &trainable : graph_ptr->trainable_lookup_)
  {
    // guarantee unique op name
    std::string node_name(name + "_" + trainable.first);
    std::string resolved_name;
    UpdateVariableName<OperationType>(node_name, resolved_name);

    trainable_.emplace_back(graph_ptr->trainable_.at(trainable.second));
    trainable_lookup_[resolved_name] = trainable_.size() - 1;
  }
}

/**
 * If AddNode is called for a non-trainable op, this version of the function is called which
 * does not append to the trainable map
 * @tparam OperationType
 * @param name
 * @param op
 * @return
 */
template <typename ArrayType>
template <class OperationType>
meta::IfIsNotGraphOrTrainable<ArrayType, OperationType, void> Graph<ArrayType>::AddTrainable(
    std::string const &name, std::shared_ptr<Node<ArrayType>> op)
{
  FETCH_UNUSED(name);
  FETCH_UNUSED(op);
}

/**
 * generates a new variable name if necessary to ensure uniqueness within graph
 * @param pre_string
 * @return
 */
template <typename ArrayType>
template <typename OperationType>
bool Graph<ArrayType>::UpdateVariableName(std::string const &name, std::string &ret)
{
  ret                       = name;
  std::string op_descriptor = (OperationType::DESCRIPTOR);
  bool        is_duplicate  = false;
  // search graph for existing variable names
  if (ret.empty())
  {  // if no name is specified, generate a default name
    std::uint64_t name_idx = 0;
    ret                    = op_descriptor + "_" + std::to_string(name_idx);
    while (!(nodes_.find(ret) == nodes_.end()))
    {
      ++name_idx;
      ret = op_descriptor + "_" + std::to_string(name_idx);
    }
  }
  else if (nodes_.find(ret) != nodes_.end())
  {  // if a duplicated name is specified, shared weight is assumed
    is_duplicate           = true;
    std::uint64_t name_idx = 1;
    ret                    = name + "_Copy_" + std::to_string(name_idx);
    while (!(nodes_.find(ret) == nodes_.end()))
    {
      ++name_idx;
      ret = name + "_Copy_" + std::to_string(name_idx);
    }
  }

  return is_duplicate;
}

/**
 * Assigns all trainable pointers to vector for optimiser purpose
 * @return ret is vector containing pointers to all trainables
 */
template <typename ArrayType>
std::vector<typename std::shared_ptr<fetch::ml::ops::Trainable<ArrayType>>>
Graph<ArrayType>::GetTrainable()
{
  return trainable_;
}

template <typename ArrayType>
std::unordered_map<std::string, typename Graph<ArrayType>::SizeType>
Graph<ArrayType>::GetTrainableLookup()
{
  return trainable_lookup_;
}

}  // namespace ml
}  // namespace fetch
