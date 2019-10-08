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
#include <utility>
#include <vector>

// TODO(#1554) - we should only reset the cache for trained nodes, not all nodes
// TODO(1467) - implement validity checks on graph compilation - e.g. loss function should not
// appear in middle of graph

namespace fetch {
namespace ml {

///////////////
/// FRIENDS ///
///////////////

namespace optimisers {
template <typename TensorType>
class Optimiser;
}  // namespace optimisers
namespace model {
template <typename TensorType>
class ModelInterface;
}  // namespace model
namespace distributed_learning {
template <typename TensorType>
class TrainingClient;
}  // namespace distributed_learning

///////////////////
/// GRAPH STATE ///
///////////////////

enum class GraphState : uint8_t
{
  INVALID,       // graph described through adding nodes is not valid for compilation
  NOT_COMPILED,  // occurs whenever adding new nodes to graph
  COMPILED,      // added nodes have been link and trainables compiled
  EVALUATED,     // forward pass has been completed - ready for backprop
  BACKWARD,      // backward pass has been completed - ready for apply gradients
  UPDATED        // gradients have been applied
};

/////////////
/// GRAPH ///
/////////////

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
  using RegPtrType       = std::shared_ptr<fetch::ml::regularisers::Regulariser<T>>;
  using SPType           = GraphSaveableParams<TensorType>;
  using OpPtrType        = std::shared_ptr<fetch::ml::ops::Ops<TensorType>>;

  static constexpr char const *DESCRIPTOR = "Graph";

  virtual ~Graph() = default;
  Graph()          = default;

  //////////////////////////////
  /// public setup functions ///
  //////////////////////////////

  template <class OperationType, typename... Params>
  std::string AddNode(std::string const &node_name, std::vector<std::string> const &inputs,
                      Params... params);

  void ResetCompile();
  void Compile();

  void AddTrainable(NodePtrType node_ptr, std::string const &node_name);
  void AddTrainable(NodePtrType node_ptr, std::string const &node_name,
                    std::map<std::string, NodePtrType> &trainable_lookup);

  void SetRegularisation(RegPtrType regulariser, DataType regularisation_rate = DataType{0.0});
  bool SetRegularisation(std::string node_name, RegPtrType regulariser,
                         DataType regularisation_rate = DataType{0.0});

  ///////////////////////////////////
  /// public train/test functions ///
  ///////////////////////////////////

  void       SetInput(std::string const &node_name, TensorType const &data);
  TensorType Evaluate(std::string const &node_name, bool is_training = true);
  void       BackPropagate(std::string const &node_name, TensorType const &error_signal = {});
  void       ApplyGradients(std::vector<TensorType> &grad);

  //////////////////////////////////////////////////////
  /// public serialisation & weight export functions ///
  //////////////////////////////////////////////////////

  bool                            InsertNode(std::string const &node_name, NodePtrType node_ptr);
  GraphSaveableParams<TensorType> GetGraphSaveableParams();
  void                            SetGraphSaveableParams(GraphSaveableParams<TensorType> const &sp);
  virtual struct fetch::ml::StateDict<TensorType> StateDict();

  virtual void LoadStateDict(struct fetch::ml::StateDict<T> const &dict);

  ////////////////////////////////////
  /// public setters and accessors ///
  ////////////////////////////////////

  NodePtrType                   GetNode(std::string const &node_name) const;
  std::vector<TensorType>       GetWeightsReferences() const;
  std::vector<TensorType>       GetWeights() const;
  std::vector<TensorType>       GetGradientsReferences() const;
  std::vector<TensorType>       GetGradients() const;
  std::vector<TrainablePtrType> GetTrainables();

  ////////////////////////////////////
  /// public gradient manipulation ///
  ////////////////////////////////////

  void ResetGradients();

protected:
  std::map<std::string, NodePtrType>                            nodes_;
  std::map<std::string, NodePtrType>                            trainable_lookup_;
  std::vector<std::pair<std::string, std::vector<std::string>>> connections_;

  void       SetInputReference(std::string const &node_name, TensorType const &data);
  void       InsertSharedCopy(std::shared_ptr<Graph<TensorType>> output_ptr);
  TensorType ForwardPropagate(std::string const &node_name, bool is_training = true);

private:
  GraphState graph_state_ = GraphState::NOT_COMPILED;

  friend class optimisers::Optimiser<TensorType>;
  friend class model::ModelInterface<TensorType>;
  friend class distributed_learning::TrainingClient<TensorType>;

  TensorType ForwardImplementation(std::string const &node_name, bool is_training,
                                   bool evaluate_mode);

  template <typename OperationType>
  bool UpdateVariableName(std::string const &name, std::string &ret);

  void LinkNodesInGraph(std::string const &node_name, std::vector<std::string> const &inputs);

  template <class OperationType, typename... Params>
  meta::IfIsShareable<TensorType, OperationType, NodePtrType> DuplicateNode(
      std::string const &node_name, std::string &updated_name);

  template <class OperationType, typename... Params>
  meta::IfIsNotShareable<TensorType, OperationType, NodePtrType> DuplicateNode(
      std::string const &node_name, std::string &updated_name);

  void ResetGraphCache(bool input_size_changed, std::shared_ptr<Node<T>> n = {});

  //////////////////////////////////////////
  /// recursive implementation functions ///
  //////////////////////////////////////////

  void StateDict(fetch::ml::StateDict<TensorType> &state_dict);
  void GetTrainables(std::vector<TrainablePtrType> &ret);
  void GetWeightsReferences(std::vector<TensorType> &ret) const;
  void GetGradientsReferences(std::vector<TensorType> &ret) const;

  template <typename IteratorType>
  void ApplyGradients(IteratorType &grad_it);

  template <typename ValType, typename NodeFunc, typename GraphFunc>
  void RecursiveApply(ValType &val, NodeFunc node_func, GraphFunc graph_func) const;

  template <typename ValType, typename GraphFunc>
  //  void RecursiveApply(ValType &val,
  //                      void (Graph<TensorType>::*subgraph_func)(ValType &) const) const;
  void RecursiveApply(ValType &val, GraphFunc graph_func) const;
};

//////////////////////
/// PUBLIC METHODS ///
//////////////////////

/**
 * the interface for adding nodes to the graph.
 * The parameters are node name, names of input nodes, and any op parameters
 * @tparam TensorType
 * @tparam OperationType Type of Op node will compute
 * @tparam Params parameters for the Op construction
 * @param node_name name of the new node to add
 * @param inputs names of other nodes that feed input to this node
 * @param params input parameters for the op construction
 * @return the updated name of the node that was added. this may differ from that specified by the
 * user
 */
template <typename TensorType>
template <class OperationType, typename... Params>
std::string Graph<TensorType>::AddNode(std::string const &             node_name,
                                       std::vector<std::string> const &inputs, Params... params)
{
  graph_state_ = GraphState::NOT_COMPILED;

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
  {
    node_ptr = DuplicateNode<OperationType, Params...>(node_name, updated_name);
  }

  // put node in look up table
  nodes_[updated_name] = node_ptr;

  // define connections between nodes
  connections_.emplace_back(std::make_pair(updated_name, inputs));

  // add to trainable lookup if it is a trainable
  AddTrainable(node_ptr, node_name);

  return updated_name;
}

/**
 * Undoes the work of a previous Compile call.
 * Since compilation could be called multiple times during graph construction, this is
 * necessary to avoid duplicate connections
 * @tparam TensorType
 */
template <typename TensorType>
void Graph<TensorType>::ResetCompile()
{
  graph_state_ = GraphState::NOT_COMPILED;

  for (auto &connection : connections_)
  {
    auto node_name   = connection.first;
    auto node_inputs = connection.second;

    // remove inputs and output from the node
    nodes_.at(node_name)->ResetInputsAndOutputs();
  }
}

/**
 * uses the connections object to link together inputs to nodes
 * Having a separate compile stage allows for arbitrary order of AddNode calls
 * @tparam TensorType
 */
template <typename TensorType>
void Graph<TensorType>::Compile()
{
  switch (graph_state_)
  {
  case GraphState::COMPILED:
  case GraphState::EVALUATED:
  case GraphState::BACKWARD:
  case GraphState::UPDATED:
  {
    // graph already compiled. do nothing
    break;
  }
  case GraphState::INVALID:
  case GraphState::NOT_COMPILED:
  {
    bool valid = true;

    ResetCompile();

    // set inputs and outputs to nodes and set trainables
    for (auto &connection : connections_)
    {
      auto node_name   = connection.first;
      auto node_inputs = connection.second;
      LinkNodesInGraph(node_name, node_inputs);
    }

    // TODO(1467) - implement validity checks on graph compilation - e.g. loss function should not
    // appear in middle of graph
    if (valid)
    {
      graph_state_ = GraphState::COMPILED;
    }
    else
    {
      graph_state_ = GraphState::INVALID;
    }
    break;
  }
  default:
  {
    throw ml::exceptions::InvalidMode("cannot evaluate graph - unrecognised graph state");
  }
  }
}
/**
 * Appends op to map of trainable nodes, called by
 * @tparam TensorType
 * @param node_ptr
 * @param node_name
 */
template <typename TensorType>
void Graph<TensorType>::AddTrainable(NodePtrType node_ptr, std::string const &node_name)
{
  AddTrainable(node_ptr, node_name, trainable_lookup_);
}

/**
 * Appends op to map of trainable nodes. Called by AddNode
 * If this op is a layer/subgraph/graph then appends all trainable ops from op.trainable_
 * @tparam TensorType
 * @param node_ptr
 * @param node_name
 */
template <typename TensorType>
void Graph<TensorType>::AddTrainable(NodePtrType node_ptr, std::string const &node_name,
                                     std::map<std::string, NodePtrType> &trainable_lookup)
{
  auto op_ptr        = node_ptr->GetOp();
  auto trainable_ptr = std::dynamic_pointer_cast<fetch::ml::ops::Trainable<TensorType>>(op_ptr);

  // if its a trainable
  if (trainable_ptr)
  {
    trainable_lookup[node_name] = node_ptr;
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
  return ForwardImplementation(node_name, is_training, true);
}

/**
 * Evaluates the output of a node via a shallow copy. This is used by the optimiser
 * and isn't safe for external users.
 * @param node_name name of node to evaluate for output
 * @return a copy of the output tensor
 */
template <typename TensorType>
TensorType Graph<TensorType>::ForwardPropagate(std::string const &node_name, bool is_training)
{
  return ForwardImplementation(node_name, is_training, false);
}

/**
 * computes the forward pass. either invoked by an external and returns a deep copy of the result,
 * or invoked by a friend of graph and returns a shallow copy of the result tensor
 * @tparam TensorType
 * @param node_name
 * @param is_training
 * @param evaluate_mode if true, returns a deep copy of the result tensor
 * @return
 */
template <typename TensorType>
TensorType Graph<TensorType>::ForwardImplementation(std::string const &node_name, bool is_training,
                                                    bool evaluate_mode)
{
  Compile();

  if (nodes_.find(node_name) != nodes_.end())
  {
    switch (graph_state_)
    {
    case GraphState::INVALID:
    case GraphState::NOT_COMPILED:
    {
      throw ml::exceptions::InvalidMode("cannot compile and evaluate graph");
    }
    case GraphState::COMPILED:
    case GraphState::EVALUATED:
    case GraphState::BACKWARD:
    case GraphState::UPDATED:
    {
      graph_state_ = GraphState::EVALUATED;
      auto ret     = (*(nodes_[node_name]->Evaluate(is_training)));
      if (evaluate_mode)
      {
        return ret.Copy();
      }
      return ret;
    }
    default:
    {
      throw ml::exceptions::InvalidMode("cannot evaluate graph - unrecognised graph state");
    }
    }
  }
  else
  {
    throw ml::exceptions::InvalidMode("Cannot evaluate: node [" + node_name + "] not in graph");
  }
}

/**
 * Backpropagate given error signal through the graph
 * If no error signal is given, an empty error signal is used
 * (which is valid when backpropagating from a loss function op
 * @param node_name name of node from which to begin backprop
 * @param error_signal pointer to array containing error signal to backprop
 */
template <typename TensorType>
void Graph<TensorType>::BackPropagate(std::string const &node_name, TensorType const &error_signal)
{
  Compile();

  // check node to backprop from exists in graph
  if (nodes_.find(node_name) != nodes_.end())
  {
    switch (graph_state_)
    {
    case GraphState::INVALID:
    case GraphState::NOT_COMPILED:
    {
      throw ml::exceptions::InvalidMode("Cannot backpropagate: graph not compiled or invalid");
    }
    case GraphState::COMPILED:
    {
      throw ml::exceptions::InvalidMode(
          "Cannot backpropagate: forward pass not completed on graph");
    }
    case GraphState::EVALUATED:
    case GraphState::BACKWARD:
    case GraphState::UPDATED:
    {
      nodes_[node_name]->BackPropagate(error_signal);
      graph_state_ = GraphState::BACKWARD;
      break;
    }
    default:
    {
      throw ml::exceptions::InvalidMode("cannot backpropagate: unrecognised graph state");
    }
    }
  }
  else
  {
    throw ml::exceptions::InvalidMode("Cannot backpropagate: node [" + node_name +
                                      "] not in graph");
  }
}

/////////////////////////
/// PROTECTED METHODS ///
/////////////////////////

///////////////////////
/// PRIVATE METHODS ///
///////////////////////

/**
 * Set regularisation type and rate for all trainables in graph
 * @tparam TensorType
 * @param regularisation_type L1, L2 or NONE
 * @param regularisation_rate
 */
template <typename TensorType>
void Graph<TensorType>::SetRegularisation(RegPtrType regulariser, DataType regularisation_rate)
{
  Compile();
  for (auto &t : trainable_lookup_)
  {
    auto tmp = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t.second->GetOp());
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
  Compile();
  NodePtrType t             = trainable_lookup_.at(node_name);
  auto        trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
  trainable_ptr->SetRegularisation(regulariser, regularisation_rate);

  return true;
}

/**
 * Add gradient values to weight for each trainable
 * @param grad vector of gradient values for each trainable stored in TensorType
 */
template <typename TensorType>
void Graph<TensorType>::ApplyGradients(std::vector<TensorType> &grad)
{
  Compile();

  switch (graph_state_)
  {
  case GraphState::INVALID:
  case GraphState::NOT_COMPILED:
  case GraphState::COMPILED:
  case GraphState::EVALUATED:
  {
    throw ml::exceptions::InvalidMode(
        "cannot apply gradients: backpropagate not previously called on graph");
  }
  case GraphState::BACKWARD:
  {
    auto grad_it = grad.begin();
    ApplyGradients(grad_it);

    for (auto const &t : nodes_)
    {
      // TODO(#1554) - we should only reset the cache for trained nodes, not all nodes
      ResetGraphCache(false, t.second);
    }
    return;
  }
  case GraphState::UPDATED:
  {
    // no gradients to apply - nothing to do
    return;
  }
  default:
  {
    throw ml::exceptions::InvalidMode("cannot apply gradients: unrecognised graph state");
  }
  }
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
  gs.connections = connections_;
  for (auto const &npair : nodes_)
  {
    std::string node_name = npair.first;
    auto        node      = npair.second;

    auto nsp = node->GetNodeSaveableParams();
    gs.nodes.insert(std::make_pair(node_name, nsp));
  }

  gs.graph_state = static_cast<uint8_t>(graph_state_);
  return gs;
}

template <typename TensorType>
void Graph<TensorType>::SetGraphSaveableParams(GraphSaveableParams<TensorType> const &sp)
{
  assert(nodes_.size() == sp.connections.size());

  connections_ = sp.connections;

  // assign inputs and outputs to the nodes
  for (auto &node : sp.connections)
  {
    LinkNodesInGraph(node.first, node.second);
  }

  graph_state_ = static_cast<GraphState>(sp.graph_state);

  switch (graph_state_)
  {
  case GraphState::INVALID:
  case GraphState::NOT_COMPILED:
  case GraphState::COMPILED:
  {
    // valid graph states, nothing to do
    return;
  }
  case GraphState::EVALUATED:
  case GraphState::BACKWARD:
  case GraphState::UPDATED:
  {
    // we revert state back to compile to prevent immediate backpropagation after deserialisation
    graph_state_ = GraphState::COMPILED;
    return;
  }
  default:
  {
    throw ml::exceptions::InvalidMode("cannot setGraphSaveableParams: graph state not recognised");
  }
  }
}

/**
 * returns ptr to node if node_name refers to a node in the graph
 * @tparam TensorType
 * @param node_name
 * @return
 */
template <typename TensorType>
typename Graph<TensorType>::NodePtrType Graph<TensorType>::GetNode(
    std::string const &node_name) const
{
  NodePtrType ret = nodes_.at(node_name);
  if (!ret)
  {
    throw ml::exceptions::InvalidMode("couldn't find node [" + node_name + "] in graph!");
  }
  return ret;
}

/**
 * Assigns data to a dataholder if the node can be found in the graph.
 * Also resets the graph cache to avoid erroneous leftover outputs
 * @param node_name name of the placeholder node in the graph (must be unique)
 * @param data the pointer to a tensor to assign to the placeholder
 */
template <typename TensorType>
void Graph<TensorType>::SetInputReference(std::string const &node_name, TensorType const &data)
{
  auto dataholder =
      std::dynamic_pointer_cast<ops::DataHolder<TensorType>>(nodes_.at(node_name)->GetOp());

  if (dataholder)
  {
    bool input_size_changed = dataholder->SetData(data);
    ResetGraphCache(input_size_changed, nodes_[node_name]);
  }
  else
  {
    throw ml::exceptions::InvalidMode("No placeholder node with name [" + node_name +
                                      "] found in graph!");
  }
}

/**
 * Assigns deep copy of data to a dataholder if the node can be found in the graph.
 * @param node_name name of the placeholder node in the graph (must be unique)
 * @param data the pointer to a tensor to assign to the placeholder
 */
template <typename TensorType>
void Graph<TensorType>::SetInput(std::string const &node_name, TensorType const &data)
{
  SetInputReference(node_name, data.Copy());
}

/**
 * Resets graph cache, clearing stored evaluation outputs
 * and recursively updating the input size for all downstream nodes
 * (or for all nodes if none specified)
 */
template <typename TensorType>
void Graph<TensorType>::ResetGraphCache(bool input_size_changed, NodePtrType n)
{
  if (!n)
  {
    for (auto &node : nodes_)
    {
      node.second->ResetCache(input_size_changed);

      auto graph_pointer = std::dynamic_pointer_cast<Graph<TensorType>>(node.second->GetOp());
      if (graph_pointer)
      {
        graph_pointer->ResetGraphCache(input_size_changed);
      }
    }
  }
  else
  {
    n->ResetCache(input_size_changed);
    for (auto &node : n->GetOutputs())
    {
      ResetGraphCache(input_size_changed, node);
    }
  }
}

/**
 * Assigns all trainable parameters to a stateDict for exporting and serialising
 * @return  d is the StateDict of all trainable params
 */

template <typename TensorType>
struct fetch::ml::StateDict<TensorType> Graph<TensorType>::StateDict()
{
  Compile();
  struct fetch::ml::StateDict<TensorType> state_dict;
  StateDict(state_dict);
  return state_dict;
}

template <typename TensorType>
void Graph<TensorType>::StateDict(fetch::ml::StateDict<TensorType> &state_dict)
{

  // add trainables in this graph to state dict
  for (auto const &t : trainable_lookup_)
  {
    auto node_ptr    = t.second;
    auto op_ptr      = node_ptr->GetOp();
    auto weights_ptr = std::dynamic_pointer_cast<ops::Weights<TensorType>>(op_ptr);
    state_dict.dict_.emplace(t.first, weights_ptr->StateDict());
  }

  // add trainables in any subgraphs to state dict
  for (auto &node_pair : nodes_)
  {
    auto op_ptr    = node_pair.second->GetOp();
    auto graph_ptr = std::dynamic_pointer_cast<Graph<TensorType>>(op_ptr);

    // if its a graph
    if (graph_ptr)
    {
      graph_ptr->StateDict(state_dict);
    }
  }
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
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t.second->GetOp());
    trainable_ptr->LoadStateDict(dict.dict_.at(t.first));
  }
}

/**
 * Assigns references of all trainable weights parameters to vector
 * @return ret is vector containing values for all weights
 */
template <typename TensorType>
std::vector<TensorType> Graph<TensorType>::GetWeightsReferences() const
{
  std::vector<TensorType> ret;
  GetWeightsReferences(ret);
  return ret;
}

/**
 * Assigns all trainable weights parameters to vector
 * @return ret is vector containing values for all weights
 */
template <typename TensorType>
std::vector<TensorType> Graph<TensorType>::GetWeights() const
{
  std::vector<TensorType> shallow_copy = GetWeightsReferences();
  std::vector<TensorType> ret(shallow_copy.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = shallow_copy[i].Copy();
  }
  return ret;
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
  GetGradientsReferences(ret);
  return ret;
}

/**
 * Assigns all trainable accumulated gradient parameters to vector of TensorType for exporting and
 * serialising
 * @return ret is vector containing all gradient values
 */
template <typename TensorType>
std::vector<TensorType> Graph<TensorType>::GetGradients() const
{
  std::vector<TensorType> shallow_copy = GetGradientsReferences();
  std::vector<TensorType> ret(shallow_copy.size());
  for (std::size_t i = 0; i < ret.size(); ++i)
  {
    ret[i] = shallow_copy[i].Copy();
  }
  return ret;
}

/**
 * Sets all accumulated gradients for each trainable to zero
 * @tparam TensorType
 */
template <typename TensorType>
void Graph<TensorType>::ResetGradients()
{
  for (auto const &t : trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t.second->GetOp());
    trainable_ptr->ResetGradients();
  }
}

/**
 * Connect the new node to the current graph by setting input and output nodes to it and saving it
 * in the lookup table. Can also be used by ResetCompile to unlink previously linked nodes
 * @tparam TensorType
 * @param node_name
 * @param inputs
 * @param unlink
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
    uint64_t name_idx = 0;
    ret               = op_descriptor + "_" + std::to_string(name_idx);
    while (!(nodes_.find(ret) == nodes_.end()))
    {
      ++name_idx;
      ret = op_descriptor + "_" + std::to_string(name_idx);
    }
  }
  else if (nodes_.find(ret) != nodes_.end())
  {  // if a duplicated name is specified, shared weight is assumed
    is_duplicate      = true;
    uint64_t name_idx = 1;
    ret               = name + "_Copy_" + std::to_string(name_idx);
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
  GetTrainables(ret);
  return ret;
}

/**
 * Inserts a copy of the graph (with shared op ptrs where appropriate) into output_ptr
 * @tparam T
 * @param output_ptr shared_ptr to the new graph. Needs to not be the same as the old graph!
 */
template <class T>
void Graph<T>::InsertSharedCopy(std::shared_ptr<Graph<TensorType>> output_ptr)
{
  if (output_ptr.get() == this)
  {
    throw ml::exceptions::InvalidMode("This needs to be called with a separate ptr.");
  }

  std::shared_ptr<Graph<TensorType>> const &copyshare = output_ptr;

  // copy all the nodes, sharing the weights using MakeSharedCopy
  for (auto const &n : nodes_)
  {
    std::string node_name = n.first;
    NodePtrType node_ptr  = n.second;
    OpPtrType   op_ptr    = node_ptr->GetOp();

    OpPtrType op_copyshare = op_ptr->MakeSharedCopy(op_ptr);

    assert(copyshare->nodes_.find(node_name) == copyshare->nodes_.end());

    copyshare->nodes_[node_name] =
        std::make_shared<Node<TensorType>>(*node_ptr, node_name, op_copyshare);

    AddTrainable(node_ptr, node_name, copyshare->trainable_lookup_);
  }

  for (auto const &n : this->nodes_)
  {
    std::string                       node_name = n.first;
    std::shared_ptr<Node<TensorType>> n_ptr     = n.second;

    copyshare->LinkNodesInGraph(node_name, this->nodes_[node_name]->GetInputNames());
  }
}

template <typename TensorType>
template <class OperationType, typename... Params>
meta::IfIsShareable<TensorType, OperationType, typename Graph<TensorType>::NodePtrType>
Graph<TensorType>::DuplicateNode(std::string const &node_name, std::string &updated_name)
{
  // if name is duplicated then shared node is required
  NodePtrType target_node = GetNode(node_name);

  // get a copy (shared when appropriate) of the target node Op
  auto op_copyshare = target_node->GetOp()->MakeSharedCopy(target_node->GetOp());

  // make a new node by giving it the copied op
  return std::make_shared<Node<TensorType>>(OperationType::OpCode(), updated_name, op_copyshare);
}

template <typename TensorType>
template <class OperationType, typename... Params>
meta::IfIsNotShareable<TensorType, OperationType, typename Graph<TensorType>::NodePtrType>
Graph<TensorType>::DuplicateNode(std::string const &node_name, std::string & /* updated_name */)
{
  throw ml::exceptions::InvalidMode(
      "OperationType is not shareable. Cannot make duplicate of node named: " + node_name);
}

template <typename TensorType>
void Graph<TensorType>::GetWeightsReferences(std::vector<TensorType> &ret) const
{
  using ret_type             = std::vector<TensorType>;
  using node_func_signature  = TensorType const &(ops::Trainable<TensorType>::*)() const;
  using graph_func_signature = void (Graph<TensorType>::*)(std::vector<TensorType> &) const;

  RecursiveApply<ret_type, node_func_signature, graph_func_signature>(
      ret, &ops::Trainable<TensorType>::GetWeights, &Graph<TensorType>::GetWeightsReferences);
}

template <typename TensorType>
void Graph<TensorType>::GetTrainables(std::vector<TrainablePtrType> &ret)
{
  using ret_type             = std::vector<TrainablePtrType>;
  using graph_func_signature = void (Graph<TensorType>::*)(ret_type &);

  for (auto const &t : trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t.second->GetOp());
    ret.emplace_back(trainable_ptr);
  }

  RecursiveApply<ret_type, graph_func_signature>(ret, &Graph<TensorType>::GetTrainables);
}

/**
 * gets all gradients (by reference) from all trainables in this graph and all subgraphs
 * @tparam TensorType
 * @param ret
 */
template <typename TensorType>
void Graph<TensorType>::GetGradientsReferences(std::vector<TensorType> &ret) const
{
  using ret_type             = std::vector<TensorType>;
  using node_func_signature  = TensorType const &(ops::Trainable<TensorType>::*)() const;
  using graph_func_signature = void (Graph<TensorType>::*)(ret_type &) const;

  RecursiveApply<ret_type, node_func_signature, graph_func_signature>(
      ret, &ops::Trainable<TensorType>::GetGradientsReferences,
      &Graph<TensorType>::GetGradientsReferences);
}

template <typename TensorType>
template <typename IteratorType>
void Graph<TensorType>::ApplyGradients(IteratorType &grad_it)
{
  using graph_func_signature = void (Graph<TensorType>::*)(IteratorType &);

  for (auto const &t : trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t.second->GetOp());
    trainable_ptr->ApplyGradient(*grad_it);
    ++grad_it;
  }

  RecursiveApply<IteratorType, graph_func_signature>(grad_it, &Graph<TensorType>::ApplyGradients);
}

/**
 * RecursiveApply is used to apply a function to all trainables and collect the results,
 * and then recursively invoke this function for any nodes which are graphs. Using this
 * function guarantees the order of elements.
 * @tparam TensorType
 * @tparam ValType
 * @tparam NodeFunc
 * @tparam GraphFunc
 * @param val
 * @param node_func
 * @param graph_func
 */
template <typename TensorType>
template <typename ValType, typename NodeFunc, typename GraphFunc>
void Graph<TensorType>::RecursiveApply(ValType &val, NodeFunc node_func, GraphFunc graph_func) const
{
  for (auto const &t : trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t.second->GetOp());
    auto tensor        = ((*trainable_ptr).*node_func)();
    val.emplace_back(tensor);
  }

  RecursiveApply<ValType, GraphFunc>(val, graph_func);
}

/**
 * Recursive apply which applies the graph function only
 * @tparam TensorType
 * @tparam ValType
 * @tparam GraphFunc
 * @param val
 * @param graph_func
 */
template <typename TensorType>
template <typename ValType, typename GraphFunc>
void Graph<TensorType>::RecursiveApply(ValType &val, GraphFunc graph_func) const
{
  // get gradients from subgraphs
  for (auto &node_pair : nodes_)
  {
    auto op_ptr    = node_pair.second->GetOp();
    auto graph_ptr = std::dynamic_pointer_cast<Graph<TensorType>>(op_ptr);

    // if its a graph
    if (graph_ptr)
    {
      ((*graph_ptr).*graph_func)(val);
    }
  }
}

}  // namespace ml
}  // namespace fetch
