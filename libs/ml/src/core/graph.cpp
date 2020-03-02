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

#include "math/tensor/tensor.hpp"
#include "math/tensor/tensor_slice_iterator.hpp"
#include "ml/charge_estimation/constants.hpp"
#include "ml/charge_estimation/core/constants.hpp"
#include "ml/core/graph.hpp"
#include "ml/ops/weights.hpp"

#include <unordered_set>

namespace fetch {

namespace ml {

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
      ComputeAllNodeShapes();

      // RecursivelyCompleteWeightsInitialisation
      for (auto const &node_name_and_ptr : nodes_)
      {
        NodePtrType node = node_name_and_ptr.second;
        node->GetOp()->Compile();
      }

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
 * Computes and/or deduces layer output shapes, recursively traversing the Graph
 * up to leaf (input) nodes. Layer output shape computation/deduction results
 * are cached in each Node.
 * @param node_name
 */
template <typename TensorType>
void Graph<TensorType>::ComputeAllNodeShapes()
{
  if (nodes_.empty() || connections_.empty())
  {
    FETCH_LOG_WARN(
        DESCRIPTOR,
        " Batch output shape computing is impossible : connection list empty or no nodes");
    return;
  }
  for (auto const &node_name_and_ptr : nodes_)
  {
    NodePtrType node = node_name_and_ptr.second;

    // A recursive call will trigger shape computing in all previous nodes or return
    // a shape if it has been already computed.
    math::SizeVector const output_shape = node->BatchOutputShape();

    if (output_shape.empty())
    {
      FETCH_LOG_WARN(DESCRIPTOR, " Batch output shape computing failed for node " +
                                     node_name_and_ptr.first + ".");
    }
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
bool Graph<TensorType>::SetRegularisation(std::string const &node_name, RegPtrType regulariser,
                                          DataType regularisation_rate)
{
  NodePtrType t             = trainable_lookup_.at(node_name);
  auto        trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t->GetOp());
  trainable_ptr->SetRegularisation(regulariser, regularisation_rate);

  return true;
}

/**
 * Set variable freezing for all trainables in graph
 * @tparam TensorType
 * @param frozen_state true=freeze variables, false=unfreeze variables
 */
template <typename TensorType>
void Graph<TensorType>::SetFrozenState(bool frozen_state)
{
  for (auto &curent_trainable : GetTrainables())
  {
    curent_trainable->SetFrozenState(frozen_state);
  }
}

/**
 * Set variable freezing for specified trainable or graph by its name
 * @tparam TensorType
 * @param node_name name of specific trainable
 * @param frozen_state true=freeze variables, false=unfreeze variables
 */
template <typename TensorType>
bool Graph<TensorType>::SetFrozenState(std::string const &node_name, bool frozen_state)
{
  OpPtrType target_node_op = GetNode(node_name)->GetOp();
  auto *trainable_ptr = dynamic_cast<fetch::ml::ops::Trainable<TensorType> *>(target_node_op.get());
  auto *graph_ptr     = dynamic_cast<fetch::ml::Graph<TensorType> *>(target_node_op.get());

  if (trainable_ptr)
  {
    trainable_ptr->SetFrozenState(frozen_state);
  }
  else if (graph_ptr)
  {
    graph_ptr->SetFrozenState(frozen_state);
  }
  else
  {
    throw exceptions::InvalidMode("Node is not graph or trainable");
  }

  return true;
}

/**
 * Add gradient values to weight for each trainable
 * @param grad vector of gradient values for each trainable stored in TensorType
 */
template <typename TensorType>
void Graph<TensorType>::ApplyGradients(std::vector<TensorType> &grad)
{
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

    // TODO(#1554) - we should only reset the cache for trained nodes, not all nodes
    // reset cache on all nodes
    for (auto const &t : nodes_)
    {
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
 * Add sparse gradient values to weight for each trainable
 * @tparam TensorType
 * @param grad vector of gradient values for each trainable stored in TensorType
 * @param update_rows vector of sets of rows to update for each trainable stored in SizeSet
 */
template <typename TensorType>
void Graph<TensorType>::ApplySparseGradients(std::vector<TensorType> &grad,
                                             std::vector<SizeSet> &   update_rows)
{
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
    auto rows_it = update_rows.begin();
    ApplySparseGradients(grad_it, rows_it);

    // TODO(#1554) - we should only reset the cache for trained nodes, not all nodes
    // reset cache on all nodes
    for (auto const &t : nodes_)
    {
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
  SetInputReference(node_name, data);
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
      if (auto ptr = node.lock())
      {
        ResetGraphCache(input_size_changed, ptr);
      }
      else
      {
        throw std::runtime_error("Unable to lock weak pointer.");
      }
    }
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
 * Assigns sets of all trainable updated rows of their gradients to vector for exporting and
 * serialising
 * @tparam TensorType
 * @return ret is vector containing indices of all updated rows stored in unordered_set for each
 * trainable
 */
template <typename TensorType>
std::vector<std::unordered_set<fetch::math::SizeType>> Graph<TensorType>::GetUpdatedRowsReferences()
    const
{
  std::vector<SizeSet> ret;
  GetUpdatedRowsReferences(ret);
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
void Graph<TensorType>::GetUpdatedRowsReferences(std::vector<SizeSet> &ret) const
{
  using ret_type             = std::vector<std::unordered_set<SizeType>>;
  using node_func_signature  = SizeSet const &(ops::Trainable<TensorType>::*)() const;
  using graph_func_signature = void (Graph<TensorType>::*)(ret_type &) const;

  RecursiveApply<ret_type, node_func_signature, graph_func_signature>(
      ret, &ops::Trainable<TensorType>::GetUpdatedRowsReferences,
      &Graph<TensorType>::GetUpdatedRowsReferences);
}

template <typename TensorType>
template <typename TensorIteratorType, typename VectorIteratorType>
void Graph<TensorType>::ApplySparseGradients(TensorIteratorType &grad_it,
                                             VectorIteratorType &rows_it)
{
  using graph_func_signature =
      void (Graph<TensorType>::*)(TensorIteratorType &, VectorIteratorType &);

  for (auto const &t : trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t.second->GetOp());
    trainable_ptr->ApplySparseGradient(*grad_it, *rows_it);
    ++grad_it;
    ++rows_it;
  }

  RecursiveApplyTwo<TensorIteratorType, VectorIteratorType, graph_func_signature>(
      grad_it, rows_it, &Graph<TensorType>::ApplySparseGradients);
}

template <typename TensorType>
template <typename TensorIteratorType>
void Graph<TensorType>::ApplyGradients(TensorIteratorType &grad_it)
{
  using graph_func_signature = void (Graph<TensorType>::*)(TensorIteratorType &);

  for (auto const &t : trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t.second->GetOp());
    trainable_ptr->ApplyGradient(*grad_it);
    ++grad_it;
  }

  RecursiveApply<TensorIteratorType, graph_func_signature>(grad_it,
                                                           &Graph<TensorType>::ApplyGradients);
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

    // if it's a graph
    if (graph_ptr)
    {
      ((*graph_ptr).*graph_func)(val);
    }
  }
}

/**
 * RecursiveApplyTwo is used to apply a function with two inputs to all trainables and collect the
 * results, and then recursively invoke this function for any nodes which are graphs. Using this
 * function guarantees the order of elements.
 * @tparam TensorType
 * @tparam Val1Type
 * @tparam Val2Type
 * @tparam NodeFunc
 * @tparam GraphFunc
 * @param val_1
 * @param val_2
 * @param node_func
 * @param graph_func
 */
template <typename TensorType>
template <typename Val1Type, typename Val2Type, typename NodeFunc, typename GraphFunc>
void Graph<TensorType>::RecursiveApplyTwo(Val1Type &val_1, Val2Type &val_2, NodeFunc node_func,
                                          GraphFunc graph_func) const
{
  for (auto const &t : trainable_lookup_)
  {
    auto trainable_ptr = std::dynamic_pointer_cast<ops::Trainable<TensorType>>(t.second->GetOp());
    auto tensor        = ((*trainable_ptr).*node_func)();
    val_1.emplace_back(tensor);
  }

  RecursiveApplyTwo<Val1Type, Val2Type, GraphFunc>(val_1, val_2, graph_func);
}

/**
 * Two inputs version of Recursive apply which applies the graph function only
 * @tparam TensorType
 * @tparam Val1Type
 * @tparam Val2Type
 * @tparam GraphFunc
 * @param val_1
 * @param val_2
 * @param graph_func
 */
template <typename TensorType>
template <typename Val1Type, typename Val2Type, typename GraphFunc>
void Graph<TensorType>::RecursiveApplyTwo(Val1Type &val_1, Val2Type &val_2,
                                          GraphFunc graph_func) const
{
  // get gradients from subgraphs
  for (auto &node_pair : nodes_)
  {
    auto op_ptr    = node_pair.second->GetOp();
    auto graph_ptr = std::dynamic_pointer_cast<Graph<TensorType>>(op_ptr);

    // if it's a graph
    if (graph_ptr)
    {
      ((*graph_ptr).*graph_func)(val_1, val_2);
    }
  }
}

/**
 * Recursively search graph and return pointer to node
 * @param name std::string name of node in format GRAPH1/...SUBGRAPHS../LEAF
 * @return Node pointer
 */
template <typename TensorType>
typename Graph<TensorType>::NodePtrType Graph<TensorType>::GetNode(
    std::string const &node_name) const
{
  std::string delimiter      = "/";
  SizeType    next_delimiter = node_name.find(delimiter);

  // false = Graph continues
  bool leaf = false;

  std::string token;

  // If there is no slash in name, name is leaf node name
  if (next_delimiter == fetch::math::numeric_max<SizeType>())
  {
    token = node_name;
    leaf  = true;
  }
  // save name before first delimiter
  else
  {
    token = node_name.substr(0, next_delimiter);
  }

  NodePtrType ret = nodes_.at(token);
  if (!ret)
  {
    throw ml::exceptions::InvalidMode("couldn't find node [" + node_name + "] in graph!");
  }

  // Node address ends with leaf node
  if (leaf)
  {
    return ret;
  }

  auto op_ptr    = ret->GetOp();
  auto graph_ptr = std::dynamic_pointer_cast<Graph<TensorType>>(op_ptr);

  // Node is not graph and node address continues
  if (!graph_ptr)
  {
    throw ml::exceptions::InvalidMode("[" + node_name + "] is not a graph!");
  }

  if (next_delimiter + 1 >= node_name.size() - 1)
  {
    throw ml::exceptions::InvalidMode("[" + node_name + "] has invalid format!");
  }

  // Continue recursive node search
  return graph_ptr->GetNode(node_name.substr(next_delimiter + 1, node_name.size() - 1));
}

/**
 * Recursively search graph and return list of all specific node names
 * @tparam TensorType
 * @tparam LookupFunction Function that returns list of current graph's nodes (trainable or all)
 * @param ret std::string name of node in format GRAPH1/...SUBGRAPHS../LEAF
 * @param lookup_function
 * @param level helper variable to remember on which level in graph function is
 */
template <typename TensorType>
template <typename LookupFunction>
void Graph<TensorType>::GetNamesRecursively(std::vector<std::string> &ret,
                                            LookupFunction            lookup_function,
                                            std::string const &       level)
{
  for (auto const &t : (this->*lookup_function)())
  {
    if (level.empty())
    {
      ret.push_back(t.first);
    }
    else
    {
      ret.push_back(level + "/" + t.first);
    }
  }

  // Recursive apply on all subgraphs
  for (auto &node_pair : nodes_)
  {
    auto op_ptr = node_pair.second->GetOp();

    auto graph_ptr = std::dynamic_pointer_cast<Graph<TensorType>>(op_ptr);

    // if it's a graph
    if (graph_ptr)
    {
      std::string next_level;
      if (level.empty())
      {
        next_level = node_pair.first;
      }
      else
      {
        next_level = level + "/" + node_pair.first;
      }

      ((*graph_ptr).GetNamesRecursively)(ret, lookup_function, next_level);
    }
  }
}

/**
 * Return list of all trainable node names in format GRAPH1/...SUBGRAPHS../LEAF
 * @return std::vector<std::string> list of names of all trainables in all subgraphs
 */
template <typename TensorType>
std::vector<std::string> Graph<TensorType>::GetTrainableNames()
{
  using graph_func_signature = std::map<std::string, NodePtrType> &(Graph<TensorType>::*)();

  std::vector<std::string> ret;
  GetNamesRecursively<graph_func_signature>(ret, &Graph<TensorType>::GetTrainableLookup);
  return ret;
}

template <typename TensorType>
std::vector<std::pair<std::string, std::vector<std::string>>> Graph<TensorType>::Connections()
{
  return connections_;
}

template <typename TensorType>
fetch::ml::OperationsCount Graph<TensorType>::ChargeForward(const std::string &node_name) const
{
  if (nodes_.find(node_name) == nodes_.end())
  {
    FETCH_LOG_ERROR(DESCRIPTOR, "The graph does not contain a node with name " + node_name);
    return 0;
  }

  if (this->graph_state_ == GraphState::NOT_COMPILED)
  {
    throw fetch::ml::exceptions::InvalidMode(
        "Can not compute charge estimate for a forward pass: graph is not compiled.");
  }

  NodePtrType                     node = nodes_.at(node_name);
  std::unordered_set<std::string> visited_nodes;
  auto                            cost_and_outputshape = node->ChargeForward(visited_nodes);
  return cost_and_outputshape.first;
}

template <typename TensorType>
OperationsCount Graph<TensorType>::ChargeBackward(const std::string &node_name) const
{
  if (nodes_.find(node_name) == nodes_.end())
  {
    FETCH_LOG_ERROR(DESCRIPTOR, "The graph does not contain a node with name " + node_name);
    return 0;
  }

  if (this->graph_state_ == GraphState::NOT_COMPILED)
  {
    throw fetch::ml::exceptions::InvalidMode(
        "Can not compute charge estimate for a backward pass: graph is not compiled.");
  }

  NodePtrType                     node = nodes_.at(node_name);
  std::unordered_set<std::string> visited_nodes;
  return node->ChargeBackward(visited_nodes).first;
}

template <typename TensorType>
OperationsCount Graph<TensorType>::ChargeCompile()
{
  OperationsCount op_cnt{charge_estimation::FUNCTION_CALL_COST};

  switch (graph_state_)
  {
  case GraphState::COMPILED:
  case GraphState::EVALUATED:
  case GraphState::BACKWARD:
  case GraphState::UPDATED:
  {
    // graph already compiled. do nothing
    return op_cnt;
  }
  case GraphState::INVALID:
  case GraphState::NOT_COMPILED:
  {
    // ResetCompile();, LinkNodesInGraph(node_name, node_inputs);
    op_cnt += charge_estimation::GRAPH_N_LINK_NODES * connections_.size();

    // These calls are necessary for optimiser charge estimation
    for (auto &connection : connections_)
    {
      auto node_name   = connection.first;
      auto node_inputs = connection.second;
      LinkNodesInGraph(node_name, node_inputs);
    }
    ComputeAllNodeShapes();

    // ComputeAllNodeShapes();
    op_cnt += nodes_.size();

    // RecursivelyCompleteWeightsInitialisation
    for (auto const &node_name_and_ptr : nodes_)
    {
      NodePtrType node = node_name_and_ptr.second;
      op_cnt           = node->GetOp()->ChargeCompile();
    }

    // graph_state_ = GraphState::COMPILED;
    op_cnt += charge_estimation::SET_FLAG;
    break;
  }
  default:
  {
    throw ml::exceptions::InvalidMode("cannot evaluate graph - unrecognised graph state");
  }
  }

  return op_cnt;
}

/**
 * Return list of all node names in format GRAPH1/...SUBGRAPHS../LEAF
 * @return std::vector<std::string> list of names of all nodes in all subgraphs
 */
template <typename TensorType>
std::vector<std::string> Graph<TensorType>::GetNodeNames()
{
  using graph_func_signature = std::map<std::string, NodePtrType> &(Graph<TensorType>::*)();

  std::vector<std::string> ret;
  GetNamesRecursively<graph_func_signature>(ret, &Graph<TensorType>::GetNodesLookup);
  return ret;
}

template <typename TensorType>
bool Graph<TensorType>::IsValidNodeName(std::string const &node_name) const
{
  // Slash is used as special character for addressing subgraphs
  return node_name.find('/') == std::string::npos;
}

template <typename TensorType>
std::map<std::string, typename Graph<TensorType>::NodePtrType>
    &Graph<TensorType>::GetTrainableLookup()
{
  return trainable_lookup_;
}

template <typename TensorType>
std::map<std::string, typename Graph<TensorType>::NodePtrType> &Graph<TensorType>::GetNodesLookup()
{
  return nodes_;
}

/**
 * Assign tensor to weight addressed by node_name
 * @tparam TensorType
 * @param node_name std::string name of weight in format GRAPH1/...SUBGRAPHS../LEAF
 * @param data tensor which will be assigned to target weight
 */
template <class TensorType>
void Graph<TensorType>::SetWeight(std::string const &node_name, TensorType const &data)
{
  auto node_ptr = GetNode(node_name);
  auto op_ptr   = std::dynamic_pointer_cast<fetch::ml::ops::Weights<TensorType>>(node_ptr->GetOp());
  if (!op_ptr)
  {
    throw ml::exceptions::InvalidMode("[" + node_name + "] is not Weight type!");
  }
  op_ptr->SetWeights(data);
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template class Graph<math::Tensor<int8_t>>;
template class Graph<math::Tensor<int16_t>>;
template class Graph<math::Tensor<int32_t>>;
template class Graph<math::Tensor<int64_t>>;
template class Graph<math::Tensor<float>>;
template class Graph<math::Tensor<double>>;
template class Graph<math::Tensor<fixed_point::fp32_t>>;
template class Graph<math::Tensor<fixed_point::fp64_t>>;
template class Graph<math::Tensor<fixed_point::fp128_t>>;

}  // namespace ml
}  // namespace fetch
