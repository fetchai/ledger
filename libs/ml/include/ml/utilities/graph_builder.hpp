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

//#include "ml/graph.hpp"
//#include "ml/layers/fully_connected.hpp"
//#include "ml/layers/convolution_1d.hpp"
//#include "ml/layers/convolution_2d.hpp"
//#include "ml/layers/PRelu.hpp"
//#include "ml/layers/self_attention.hpp"
//#include "ml/layers/skip_gram.hpp"

#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/abs.hpp"

//#include "ml/ops/activations/dropout.hpp"
//#include "ml/ops/activations/elu.hpp"
//#include "ml/ops/activations/leaky_relu.hpp"
//#include "ml/ops/activations/logsigmoid.hpp"
//#include "ml/ops/activations/logsoftmax.hpp"
//#include "ml/ops/activations/randomised_relu.hpp"
//#include "ml/ops/activations/relu.hpp"
//#include "ml/ops/activations/sigmoid.hpp"
//#include "ml/ops/activations/softmax.hpp"
//#include "ml/ops/add.hpp"
//#include "ml/ops/concatenate.hpp"
//#include "ml/ops/convolution_1d.hpp"
//#include "ml/ops/convolution_2d.hpp"
//#include "ml/ops/divide.hpp"
//#include "ml/ops/embeddings.hpp"
//#include "ml/ops/exp.hpp"
//#include "ml/ops/flatten.hpp"
//#include "ml/ops/log.hpp"
//#include "ml/ops/loss_functions.hpp"
//#include "ml/ops/matrix_multiply.hpp"
//#include "ml/ops/max_pool_1d.hpp"
//#include "ml/ops/max_pool_2d.hpp"
//#include "ml/ops/maximum.hpp"
//#include "ml/ops/multiply.hpp"
//#include "ml/ops/placeholder.hpp"
//#include "ml/ops/reshape.hpp"
//#include "ml/ops/sqrt.hpp"
//#include "ml/ops/subtract.hpp"
//#include "ml/ops/tanh.hpp"
//#include "ml/ops/transpose.hpp"
//#include "ml/ops/weights.hpp"
//#include "ml/saveparams/saveable_params.hpp"
//
//#include <typeindex>

namespace fetch {
namespace ml {
namespace utilities {

template <typename GraphType, typename Op>
void AddNode(std::shared_ptr<GraphType> g_ptr, std::shared_ptr<SaveableParamsInterface> saved_node,
             std::string node_name, std::vector<std::string> const &inputs)
{
  auto castnode = std::dynamic_pointer_cast<typename Op::SPType>(saved_node);
  if (!castnode)
  {
    throw std::runtime_error("Node downcasting failed.");
  }
  g_ptr->template AddNode<Op>(node_name, inputs, *castnode);
}

// TODO: this should be recursive
// when you want to add a subgraph call this with the subgraph as GraphType
// Use AddNode to add nodes that are not graphs
// MyAddSubgraph currently does nothing! But should call this.
// Pass in a pointer to the current graph to graph reconstructor.
// TODO: self_attention need getopsaveableparams
// TODO: tests for all the layers

/**
 * method for adding an Abs node to a graph based on save params.
 * @tparam TensorType
 * @tparam Descriptor
 * @param g_ptr
 * @param saved_node
 * @param node_name
 * @param inputs
 * @return
 */
template <typename TensorType, OpType OperationType>
fetch::ml::meta::IfOpIsAbs<OperationType, void> AddToGraph(
    std::shared_ptr<Graph<TensorType>>              g_ptr,
    std::shared_ptr<SaveableParamsInterface> const &saved_node, std::string const &node_name,
    std::vector<std::string> const &inputs)
{
  auto castnode =
      std::dynamic_pointer_cast<typename fetch::ml::ops::Abs<TensorType>::SPType>(saved_node);
  if (!castnode)
  {
    throw std::runtime_error("Node downcasting failed.");
  }
  g_ptr->template AddNode<fetch::ml::ops::Abs<TensorType>>(node_name, inputs, *castnode);
}

/**
 * method for constructing a graph given saveparams
 * @tparam TensorType
 * @tparam GraphType
 * @param gsp shared pointer to a graph
 * @return
 */
template <typename TensorType, typename GraphType>
GraphType LoadGraph(std::shared_ptr<GraphSaveableParams<TensorType>> gsp)
{
  // set up the return graph
  auto ret = std::make_shared<GraphType>();

  // loop through nodenames adding nodes
  for (auto &node : gsp->connections)
  {
    std::string              name   = node.first;
    std::vector<std::string> inputs = node.second;

    fetch::ml::utilities::AddToGraph<TensorType, gsp->nodes[name].OP_DESCRIPTOR>(
        ret, gsp->nodes[name], name, inputs);
  }
  return *ret;
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
