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
#include "ml/core/node.hpp"
#include "ml/ops/weights.hpp"
#include "ml/ops/op_interface.hpp"

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

  template <class Op, typename... Params>
  std::string AddNode(std::string const &node_name, std::vector<std::string> const &inputs, Params... params);


  NodePtrType GetNode(std::string const &node_name) const;
  void        SetInput(std::string const &node_name, ArrayType data);
  void        ResetGraphCache(std::shared_ptr<Node<T>> const &n, bool input_size_changed);
  void        SetRegularisation(fetch::ml::details::RegularisationType regularisation_type,
                                DataType regularisation_rate = DataType{0.0});
  bool        SetRegularisation(std::string                            node_name,
                                fetch::ml::details::RegularisationType regularisation_type,
                                DataType regularisation_rate = DataType{0.0});

  virtual struct fetch::ml::StateDict<ArrayType> StateDict() const;
  virtual void LoadStateDict(struct fetch::ml::StateDict<T> const &dict);

  std::vector<ArrayType>        get_weights() const;
  std::vector<ArrayType>        GetGradients() const;
  void                          ApplyGradients(std::vector<ArrayType> &grad);
  std::vector<TrainablePtrType> get_trainables();

  void                           ResetGradients();
  GraphSaveableParams<ArrayType> GetGraphSaveableParams();

  static constexpr char const *DESCRIPTOR = "Graph";

private:
  void ApplyRegularisation();

  std::string UpdateVariableName(OpType const & operation_type, std::string const &name);

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
void Graph<ArrayType>::SetRegularisation(fetch::ml::details::RegularisationType regularisation_type,
                                         DataType                               regularisation_rate)
{
  for (auto &t : trainable_)
  {
    t->SetRegularisation(regularisation_type, regularisation_rate);
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
bool Graph<ArrayType>::SetRegularisation(std::string                            node_name,
                                         fetch::ml::details::RegularisationType regularisation_type,
                                         DataType                               regularisation_rate)
{
  if (nodes_[node_name])
  {
    trainable_[node_name]->SetRegularisation(regularisation_type, regularisation_rate);
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
std::string Graph<ArrayType>::AddNode(std::string const &node_name, std::vector<std::string> const &inputs, Params... params)
{
  // guarantee unique op name
  std::string updated_name;
  bool        is_duplicate = UpdateVariableName<OperationType>(node_name, updated_name);

  // Instantiate the node
  auto op = std::make_shared<Node<ArrayType>>(OperationType::OpCode(), [node_name, params ...](){return new OperationType(node_name, params...);});

  if (!is_duplicate)
  {
    // Instantiate the node based on params
    op = std::make_shared<Node<ArrayType>>(updated_name, params...);
  }
  else
  {  // if shared weight is specified by duplicate naming
    // Instantiate the node based on pointer to shared target node
    NodePtrType target_node = GetNode(node_name);

    op = std::make_shared<Node<ArrayType>>(updated_name, target_node, params...);
  }

  // assign inputs and outputs to the new node
  LinkNodesInGraph(updated_name, inputs, op);

  // add to map of trainable ops if necessary
  AddTrainable(updated_name, op);

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
 * generates a new variable name if necessary to ensure uniqueness within graph
 * @param pre_string
 * @return
 */
template <typename ArrayType>
template <typename OperationType>
bool Graph<ArrayType>::UpdateVariableName(std::string const &name, std::string &ret)
{
  std::string ret           = name;
  // search graph for existing variable names
  if ((nodes_.find(ret) != nodes_.end()) || ret.empty())
  {
    std::uint64_t name_idx = 0;
    while (!(nodes_.find(ret) == nodes_.end()))
    {
      ret = ops::OpInterface::Descriptor(operation_type) + "_" + std::to_string(name_idx);
      // TODO - a timeout would be nice
      ++name_idx;
    }
  }

  return ret;
}

/**
 * Assigns all trainable pointers to vector for optimiser purpose
 * @return ret is vector containing pointers to all trainables
 */
template <typename ArrayType>
std::vector<typename std::shared_ptr<fetch::ml::ops::Trainable<ArrayType>>>
Graph<ArrayType>::get_trainables()
{
  return trainable_;
}

}  // namespace ml
}  // namespace fetch
