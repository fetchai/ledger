#pragma once
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

#include "ml/core/node.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "ml/exceptions/exceptions.hpp"
//#include <map>
#include "ml/state_dict.hpp"

// TODO(#1554) - we should only reset the cache for trained nodes, not all nodes
// TODO(1467) - implement validity checks on graph compilation - e.g. loss function should not
// appear in middle of graph

namespace fetch {

namespace dmlf {
namespace collective_learning {
template <typename TensorType>
class ClientAlgorithm;
}  // namespace collective_learning
}  // namespace dmlf

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

template <typename T>
struct StateDict;

template <class T>
class Graph
{
public:
  using TensorType       = T;
  using ArrayPtrType     = std::shared_ptr<TensorType>;
  using SizeType         = fetch::math::SizeType;
  using SizeSet          = std::unordered_set<SizeType>;
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

  void SetRegularisation(RegPtrType regulariser, DataType regularisation_rate = DataType{0});
  bool SetRegularisation(std::string const &node_name, RegPtrType regulariser,
                         DataType regularisation_rate = DataType{0});

  void SetFrozenState(bool frozen_state);
  bool SetFrozenState(std::string const &node_name, bool frozen_state);

  ///////////////////////////////////
  /// public train/test functions ///
  ///////////////////////////////////

  void       SetInput(std::string const &node_name, TensorType const &data);
  TensorType Evaluate(std::string const &node_name, bool is_training = true);
  void       BackPropagate(std::string const &node_name, TensorType const &error_signal = {});
  void       ApplyGradients(std::vector<TensorType> &grad);
  void       ApplySparseGradients(std::vector<TensorType> &grad, std::vector<SizeSet> &update_rows);

  //////////////////////////////////////////////////////
  /// public serialisation & weight export functions ///
  //////////////////////////////////////////////////////

  bool                            InsertNode(std::string const &node_name, NodePtrType node_ptr);
  GraphSaveableParams<TensorType> GetGraphSaveableParams();
  void                            SetGraphSaveableParams(GraphSaveableParams<TensorType> const &sp);
  virtual fetch::ml::StateDict<TensorType> StateDict();
  virtual void                             LoadStateDict(fetch::ml::StateDict<T> const &dict);

  ////////////////////////////////////
  /// public setters and accessors ///
  ////////////////////////////////////

  NodePtrType                   GetNode(std::string const &node_name) const;
  std::vector<TensorType>       GetWeightsReferences() const;
  std::vector<TensorType>       GetWeights() const;
  std::vector<TensorType>       GetGradientsReferences() const;
  std::vector<SizeSet>          GetUpdatedRowsReferences() const;
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
  friend class dmlf::collective_learning::ClientAlgorithm<TensorType>;

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
  void GetUpdatedRowsReferences(std::vector<SizeSet> &ret) const;

  template <typename TensorIteratorType>
  void ApplyGradients(TensorIteratorType &grad_it);

  template <typename TensorIteratorType, typename VectorIteratorType>
  void ApplySparseGradients(TensorIteratorType &grad_it, VectorIteratorType &rows_it);

  template <typename ValType, typename NodeFunc, typename GraphFunc>
  void RecursiveApply(ValType &val, NodeFunc node_func, GraphFunc graph_func) const;

  template <typename ValType, typename GraphFunc>
  void RecursiveApply(ValType &val, GraphFunc graph_func) const;

  template <typename Val1Type, typename Val2Type, typename NodeFunc, typename GraphFunc>
  void RecursiveApplyTwo(Val1Type &val_1, Val2Type &val_2, NodeFunc node_func,
                         GraphFunc graph_func) const;

  template <typename Val1Type, typename Val2Type, typename GraphFunc>
  void RecursiveApplyTwo(Val1Type &val_1, Val2Type &val_2, GraphFunc graph_func) const;
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

}  // namespace ml
}  // namespace fetch
