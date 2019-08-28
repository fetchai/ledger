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
  using TensorType       = T;
  using ArrayPtrType     = std::shared_ptr<TensorType>;
  using SizeType         = typename TensorType::SizeType;
  using DataType         = typename TensorType::Type;
  using NodePtrType      = typename std::shared_ptr<fetch::ml::Node<TensorType>>;
  using TrainablePtrType = typename std::shared_ptr<fetch::ml::ops::Trainable<TensorType>>;
  using PlaceholderType  = typename fetch::ml::ops::PlaceHolder<TensorType>;
  using RegPtrType       = std::shared_ptr<fetch::ml::regularisers::Regulariser<T>>;
  using SPType           = GraphSaveableParams<TensorType>;

  virtual ~Graph() = default;
  Graph()          = default;

  TensorType Evaluate(std::string const &node_name, bool is_training = true);
  TensorType ForwardPropagate(std::string const &node_name, bool is_training = true);

  void         BackPropagateSignal(std::string const &node_name, TensorType const &error_signal);
  void         BackPropagateError(std::string const &node_name);
  virtual void Step(DataType learning_rate);

  template <class OperationType, typename... Params>
  meta::IfIsNotShareable<TensorType, OperationType, std::string> AddNode(
      std::string const &node_name, std::vector<std::string> const &inputs, Params... params);

  template <class OperationType, typename... Params>
  meta::IfIsShareable<TensorType, OperationType, std::string> AddNode(
      std::string const &node_name, std::vector<std::string> const &inputs, Params... params);

  bool InsertNode(std::string const &node_name, NodePtrType node_ptr);

  NodePtrType GetNode(std::string const &node_name) const;
  void        SetInput(std::string const &node_name, TensorType data);
  void        ResetGraphCache(std::shared_ptr<Node<T>> const &n, bool input_size_changed);
  void SetRegularisation(RegPtrType regulariser, DataType regularisation_rate = DataType{0.0});
  bool SetRegularisation(std::string node_name, RegPtrType regulariser,
                         DataType regularisation_rate = DataType{0.0});

  virtual struct fetch::ml::StateDict<TensorType> StateDict() const;
  virtual void LoadStateDict(struct fetch::ml::StateDict<T> const &dict);

  std::vector<TensorType> get_weights() const;
  void                    SetWeights(std::vector<TensorType> &new_weights);
  std::vector<TensorType> GetGradientsReferences() const;
  std::vector<TensorType> GetGradients() const;
  void                    ApplyGradients(std::vector<TensorType> &grad);

  std::vector<TrainablePtrType> GetTrainables();

  void                            ResetGradients();
  GraphSaveableParams<TensorType> GetGraphSaveableParams();
  void                            SetGraphSaveableParams(GraphSaveableParams<TensorType> const &sp);
  static constexpr char const *   DESCRIPTOR = "Graph";

  template <class OperationType>
  meta::IfIsTrainable<TensorType, OperationType, void> AddTrainable(
      std::string const &name, std::shared_ptr<Node<TensorType>> node_ptr);

  template <class OperationType>
  meta::IfIsGraph<TensorType, OperationType, void> AddTrainable(
      std::string const &name, std::shared_ptr<Node<TensorType>> node_ptr);

  template <class OperationType>
  meta::IfIsNotGraphOrTrainable<TensorType, OperationType, void> AddTrainable(
      std::string const &name, std::shared_ptr<Node<TensorType>> node_ptr);

  void AddExternalGradients(std::vector<TensorType> grads);

private:
  void ApplyRegularisation();

  template <typename OperationType>
  bool UpdateVariableName(std::string const &name, std::string &ret);

  void LinkNodesInGraph(std::string const &node_name, std::vector<std::string> const &inputs);

protected:
  std::unordered_map<std::string, NodePtrType> nodes_;
  std::unordered_map<std::string, SizeType>    trainable_lookup_;
  std::vector<NodePtrType>                     trainable_nodes_;
};

/**
 * Set regularisation type and rate for all trainables in graph
 * @tparam TensorType
 * @param regularisation_type L1, L2 or NONE
 * @param regularisation_rate
 */
template <typename TensorType>
void Graph<TensorType>::SetRegularisation(RegPtrType regulariser, DataType regularisation_rate)
{
  for (auto &t : trainable_nodes_)
  {
    auto tmp = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    tmp->SetRegularisation(regulariser, regularisation_rate);
  }
}

/**
 * Set regularisation type and rate for specified trainable by it's name
 * @tparam TensorType
 * @param node_name name of specific trainable
 * @param regularisation_type L1, L2 or NONE
 * @param regularisation_rate
 */
template <typename TensorType>
bool Graph<TensorType>::SetRegularisation(std::string node_name, RegPtrType regulariser,
                                          DataType regularisation_rate)
{
  NodePtrType t             = trainable_nodes_.at(trainable_lookup_.at(node_name));
  auto        trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
  trainable_ptr->SetRegularisation(regulariser, regularisation_rate);

  return true;
}

/**
 * Evaluates the output of a node (calling all necessary forward prop)
 * @param node_name name of node to evaluate for output
 * @return a copy of the output tensor
 */
template <typename TensorType>
TensorType Graph<TensorType>::ForwardPropagate(std::string const &node_name, bool is_training)
{
  if (nodes_.find(node_name) != nodes_.end())
  {
    return ((*(nodes_[node_name]->Evaluate(is_training))));
  }
  else
  {
    throw std::runtime_error("Cannot evaluate: node [" + node_name + "] not in graph");
  }
}

/**
 * Evaluates the output of a node (calling all necessary forward prop)
 * @param node_name name of node to evaluate for output
 * @return a copy of the output tensor
 */
template <typename TensorType>
TensorType Graph<TensorType>::Evaluate(std::string const &node_name, bool is_training)
{
  if (nodes_.find(node_name) != nodes_.end())
  {
    return ((*(nodes_[node_name]->Evaluate(is_training))).Copy());
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
template <typename TensorType>
void Graph<TensorType>::BackPropagateError(std::string const &node_name)
{
  TensorType error_signal;
  if (nodes_.find(node_name) != nodes_.end())
  {
    nodes_[node_name]->BackPropagateSignal(error_signal);
  }
  else
  {
    throw std::runtime_error("Cannot backpropagate: node [" + node_name + "] not in graph");
  }

  // Applies regularisation to all trainables based on their configuration
  ApplyRegularisation();
}

/**
 * Backpropagate given error signal through the graph
 * @param node_name name of node from which to begin backprop
 * @param error_signal pointer to array containing error signal to backprop
 */
template <typename TensorType>
void Graph<TensorType>::BackPropagateSignal(std::string const &node_name,
                                            TensorType const & error_signal)
{
  if (nodes_.find(node_name) != nodes_.end())
  {
    nodes_[node_name]->BackPropagateSignal(error_signal);
  }
  else
  {
    throw std::runtime_error("Cannot backpropagate signal: node [" + node_name + "] not in graph");
  }
}

/**
 * takes a training step
 * @param learning_rate the learning rate (alpha) hyperparameter
 */
template <typename TensorType>
void Graph<TensorType>::Step(DataType learning_rate)
{
  for (auto &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    trainable_ptr->Step(learning_rate);
    ResetGraphCache(t, false);
  }
}

template <typename TensorType>
void Graph<TensorType>::ApplyRegularisation()
{
  for (auto &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    trainable_ptr->ApplyRegularisation();
    ResetGraphCache(t, false);  // ApplyRegularisation changes the weights so the cache is invalid
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
template <typename TensorType>
template <class OperationType, typename... Params>
meta::IfIsShareable<TensorType, OperationType, std::string> Graph<TensorType>::AddNode(
    std::string const &node_name, std::vector<std::string> const &inputs, Params... params)
{
  // guarantee unique op name
  std::string updated_name;
  bool        is_duplicate = UpdateVariableName<OperationType>(node_name, updated_name);

  NodePtrType node_ptr;
  if (!is_duplicate)
  {
    // Instantiate the node based on params
    node_ptr = std::make_shared<Node<TensorType>>(
        OperationType::OpCode(), updated_name,
        [params...]() { return std::make_shared<OperationType>(params...); });
  }
  else
  {  // if shared weight is specified by duplicate naming
    // Instantiate the node based on pointer to shared target node
    NodePtrType target_node = GetNode(node_name);
    node_ptr                = std::make_shared<Node<TensorType>>(
        OperationType::OpCode(), updated_name, [target_node, params...]() {
          return std::make_shared<OperationType>(target_node->GetOp(), params...);
        });
  }

  // put node in look up table
  nodes_[updated_name] = node_ptr;

  // assign inputs and outputs to the new node
  LinkNodesInGraph(updated_name, inputs);

  // add to map of trainable ops if necessary
  AddTrainable<OperationType>(updated_name, node_ptr);

  // return unique node name (may not be identical to node_name)
  return updated_name;
}

/**
 * Adds a node without trainable parameters.
 * @tparam OperationType Op template type
 * @tparam Params template for input parameters to node
 * @param node_name non-unique specified node name
 * @param inputs names of node inputs to the node
 * @param params input parameters to the node op
 */
template <typename TensorType>
template <class OperationType, typename... Params>
meta::IfIsNotShareable<TensorType, OperationType, std::string> Graph<TensorType>::AddNode(
    std::string const &node_name, std::vector<std::string> const &inputs, Params... params)
{
  // guarantee unique op name
  std::string updated_name;
  UpdateVariableName<OperationType>(node_name, updated_name);

  NodePtrType node_ptr;
  // Instantiate the node based on params
  node_ptr = std::make_shared<Node<TensorType>>(
      OperationType::OpCode(), updated_name,
      [params...]() { return std::make_shared<OperationType>(params...); });

  // put node in look up table
  nodes_[updated_name] = node_ptr;

  // assign inputs and outputs to the new node
  LinkNodesInGraph(updated_name, inputs);

  // add to map of trainable ops if necessary
  AddTrainable<OperationType>(updated_name, node_ptr);

  // return unique node name (may not be identical to node_name)
  return updated_name;
}

/**
 * Method for directly inserting nodes to graph - used for serialisation
 * @tparam T
 * @param node_name
 * @return
 */
template <typename T>
bool Graph<T>::InsertNode(std::string const &node_name, NodePtrType node_ptr)
{
  // put node in look up table
  nodes_[node_name] = node_ptr;
  return nodes_.find(node_name) != nodes_.end();
}

/**
 * @brief Method for constructing a graph saveable params object for serializing a Graph
 * This method constructs and returns a graph saveable params object containing all
 * necessary data for serialising and deserialising a Graph object. In order to do this
 * it must first construct a vector of connections to store, and then repeatedly call
 * the Node save params construction method
 * @tparam TensorType
 * @return GraphSaveableParams object fully defining a graph
 */
template <typename TensorType>
GraphSaveableParams<TensorType> Graph<TensorType>::GetGraphSaveableParams()
{
  GraphSaveableParams<TensorType> gs;
  for (auto const &npair : nodes_)
  {
    std::string node_name = npair.first;
    auto        node      = npair.second;

    gs.connections.emplace_back(std::make_pair(node_name, node->GetInputNames()));

    auto nsp = node->GetNodeSaveableParams();
    gs.nodes.insert(std::make_pair(node_name, nsp));
  }

  return gs;
}

template <typename TensorType>
void Graph<TensorType>::SetGraphSaveableParams(GraphSaveableParams<TensorType> const &sp)
{
  assert(nodes_.size() == sp.connections.size());

  // assign inputs and outputs to the nodes
  for (auto &node : sp.connections)
  {
    LinkNodesInGraph(node.first, node.second);
  }
}

template <typename TensorType>
typename Graph<TensorType>::NodePtrType Graph<TensorType>::GetNode(
    std::string const &node_name) const
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
template <typename TensorType>
void Graph<TensorType>::SetInput(std::string const &node_name, TensorType data)
{
  auto placeholder = std::dynamic_pointer_cast<PlaceholderType>(nodes_.at(node_name)->GetOp());

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
template <typename TensorType>
void Graph<TensorType>::ResetGraphCache(NodePtrType const &n, bool input_size_changed)
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
template <typename TensorType>
struct fetch::ml::StateDict<TensorType> Graph<TensorType>::StateDict() const
{
  struct fetch::ml::StateDict<TensorType> d;
  for (auto const &t : trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(
        trainable_nodes_.at(t.second)->GetOp());
    d.dict_.emplace(t.first, trainable_ptr->StateDict());
  }
  return d;
}

/**
 * Import trainable parameters from an exported model
 * @param dict  state dictionary to import to weights
 */
template <typename TensorType>
void Graph<TensorType>::LoadStateDict(struct fetch::ml::StateDict<TensorType> const &dict)
{
  assert(!dict.weights_);
  for (auto const &t : trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(
        trainable_nodes_.at(t.second)->GetOp());
    trainable_ptr->LoadStateDict(dict.dict_.at(t.first));
  }
}

/**
 * Assigns all trainable weights parameters to vector of TensorType for exporting and serialising
 * @return ret is vector containing values for all weights
 */
template <typename TensorType>
std::vector<TensorType> Graph<TensorType>::get_weights() const
{
  std::vector<TensorType> ret;

  for (auto const &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    ret.emplace_back(trainable_ptr->get_weights());
  }
  return ret;
}

/**
 * Write weights from vector to trainables - used for importing model
 * @tparam TensorType
 * @param new_weights
 */
template <typename TensorType>
void Graph<TensorType>::SetWeights(std::vector<TensorType> &new_weights)
{
  auto trainable_it = trainable_nodes_.begin();
  auto weights_it   = new_weights.begin();
  {
    auto trainable_ptr =
        std::dynamic_pointer_cast<ops::Trainable<TensorType>>((*trainable_it)->GetOp());
    trainable_ptr->SetWeights(*weights_it);

    ++trainable_it;
    ++weights_it;
  }
}

/**
 * Assigns all trainable accumulated gradient parameters to vector of TensorType for exporting and
 * serialising
 * @return ret is vector containing all gradient values
 */
template <typename TensorType>
std::vector<TensorType> Graph<TensorType>::GetGradientsReferences() const
{
  std::vector<TensorType> ret;

  for (auto const &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    ret.emplace_back(trainable_ptr->GetGradientsReferences());
  }
  return std::move(ret);
}

template <typename TensorType>
void Graph<TensorType>::set_weights(std::vector<TensorType> &new_weights)
{
  SizeType index = 0;
  for (auto &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    trainable_ptr->set_weights(new_weights.at(index));
    index++;
  }
}

/**
 * Assigns all trainable accumulated gradient parameters to vector of TensorType for exporting and
 * serialising
 * @return ret is vector containing all gradient values
 */
template <typename TensorType>
std::vector<TensorType> Graph<TensorType>::get_gradients_references() const
{
  std::vector<TensorType> ret;

  for (auto const &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    ret.emplace_back(trainable_ptr->get_gradients_references());
  }
  return std::move(ret);
}

/**
 * Assigns all trainable accumulated gradient parameters to vector of TensorType for exporting and
 * serialising
 * @return ret is vector containing all gradient values
 */
template <typename TensorType>
std::vector<TensorType> Graph<TensorType>::GetGradients() const
{
  std::vector<TensorType> ret;

  for (auto const &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    ret.emplace_back(trainable_ptr->GetGradients());
  }
  return ret;
}

/**
 * Sets all accumulated gradients for each trainable to zero
 */
template <typename TensorType>
void Graph<TensorType>::ResetGradients()
{
  for (auto const &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    trainable_ptr->ResetGradients();
  }
}

/**
 * Add gradient values to weight for each trainable
 * @param grad vector of gradient values for each trainable stored in TensorType
 */
template <typename TensorType>
void Graph<TensorType>::ApplyGradients(std::vector<TensorType> &grad)
{
  auto grad_it = grad.begin();
  for (auto const &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    trainable_ptr->ApplyGradient(*grad_it);
    ResetGraphCache(t, false);
    ++grad_it;
  }
}


template <typename T>
void Graph<T>::AddExternalGradients(std::vector<TensorType> grads)
{
  assert(grads.size() == trainable_nodes_.size());
  auto gt_it = trainable_nodes_.begin();
  for (auto const & grad : grads){
     gt_it->AddExternalGradient(*grad);
     ++gt_it;
  }
}


/**
 * Connect the new node to the current graph by setting input and output nodes to it and saving it
 * in the lookup table
 * @tparam TensorType
 * @tparam OperationType
 * @tparam Params
 * @param node_name
 * @param inputs
 * @param op
 */
template <typename TensorType>
void Graph<TensorType>::LinkNodesInGraph(std::string const &             node_name,
                                         std::vector<std::string> const &inputs)
{
  // assign inputs and outputs
  for (auto const &i : inputs)
  {
    nodes_.at(node_name)->AddInput(nodes_.at(i));
    nodes_[i]->AddOutput(nodes_[node_name]);
  }
}

/**
 * Appends op to map of trainable nodes. Called by AddNode if the node is for a trainable op
 * @tparam OperationType template class of operation
 * @param name the guaranteed unique name of the node
 * @param op the pointer to the op
 */
template <typename TensorType>
template <class OperationType>
meta::IfIsTrainable<TensorType, OperationType, void> Graph<TensorType>::AddTrainable(
    std::string const &name, std::shared_ptr<Node<TensorType>> node_ptr)
{
  auto op_ptr = node_ptr->GetOp();

  // it must be safe to cast this op down to a weight
  trainable_nodes_.emplace_back(node_ptr);
  trainable_lookup_[name] = trainable_nodes_.size() - 1;
}

/**
 * Appends all trainable ops from op.trainable_ to map of trainable nodes.
 * Called by AddNode if the OperationType is a graph
 * @tparam OperationType template class of operation
 * @param name the guaranteed unique name of the node
 * @param op the pointer to the op
 */
template <typename TensorType>
template <class OperationType>
meta::IfIsGraph<TensorType, OperationType, void> Graph<TensorType>::AddTrainable(
    std::string const &name, std::shared_ptr<Node<TensorType>> node_ptr)
{
  auto concrete_op_ptr = std::dynamic_pointer_cast<OperationType>(node_ptr->GetOp());

  for (auto &trainable : concrete_op_ptr->trainable_lookup_)
  {
    // guarantee unique op name
    std::string node_name(name + "_" + trainable.first);
    std::string resolved_name;
    UpdateVariableName<OperationType>(node_name, resolved_name);

    trainable_nodes_.emplace_back(concrete_op_ptr->trainable_nodes_.at(trainable.second));
    trainable_lookup_[resolved_name] = trainable_nodes_.size() - 1;
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
template <typename TensorType>
template <class OperationType>
meta::IfIsNotGraphOrTrainable<TensorType, OperationType, void> Graph<TensorType>::AddTrainable(
    std::string const &name, std::shared_ptr<Node<TensorType>> node_ptr)
{
  FETCH_UNUSED(name);
  FETCH_UNUSED(node_ptr);
}

/**
 * generates a new variable name if necessary to ensure uniqueness within graph
 * @param pre_string
 * @return
 */
template <typename TensorType>
template <typename OperationType>
bool Graph<TensorType>::UpdateVariableName(std::string const &name, std::string &ret)
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
template <typename TensorType>
std::vector<typename Graph<TensorType>::TrainablePtrType> Graph<TensorType>::GetTrainables()
{
  std::vector<TrainablePtrType> ret;
  for (auto &t : trainable_nodes_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
    ret.emplace_back(trainable_ptr);
  }
  return ret;
}

}  // namespace ml
}  // namespace fetch
