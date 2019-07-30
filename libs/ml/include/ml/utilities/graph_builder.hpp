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
#include "ml/layers/PRelu.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/self_attention.hpp"
#include "ml/layers/skip_gram.hpp"

#include "ml/meta/ml_type_traits.hpp"

#include "ml/ops/abs.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/elu.hpp"
#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/randomised_relu.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/exp.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/log.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"
#include "ml/ops/maximum.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/reshape.hpp"
#include "ml/ops/sqrt.hpp"
#include "ml/ops/subtract.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/weights.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace utilities {

// TODO: this should be recursive
// when you want to add a subgraph call this with the subgraph as GraphType
// Use AddNode to add nodes that are not graphs
// MyAddSubgraph currently does nothing! But should call this.
// Pass in a pointer to the current graph to graph reconstructor.
// TODO: self_attention need getopsaveableparams
// TODO: tests for all the layers

template <typename TensorType>
void AddToGraph(OpType const &operation_type, std::shared_ptr<Graph<TensorType>> g_ptr,
                std::shared_ptr<SaveableParamsInterface> const &saved_node,
                std::string const &node_name, std::vector<std::string> const &inputs)
{
  assert(operation_type != OpType::NONE);
  assert(operation_type != OpType::GRAPH);
  assert(operation_type != OpType::SUBGRAPH);

  switch (operation_type)
  {
  case OpType::PLACEHOLDER:
  {
    g_ptr->template AddNode<ops::PlaceHolder<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::PlaceHolder<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::WEIGHTS:
  {
    g_ptr->template AddNode<ops::Weights<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::Weights<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::DROPOUT:
  {
    g_ptr->template AddNode<ops::Dropout<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::Dropout<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::LEAKY_RELU:
  {
    g_ptr->template AddNode<ops::LeakyRelu<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::LeakyRelu<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::RANDOMISED_RELU:
  {
    g_ptr->template AddNode<ops::RandomisedRelu<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::RandomisedRelu<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::SOFTMAX:
  {
    g_ptr->template AddNode<ops::Softmax<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::Softmax<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::CONVOLUTION_1D:
  {
    g_ptr->template AddNode<ops::Convolution1D<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::Convolution1D<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::MAX_POOL_1D:
  {
    g_ptr->template AddNode<ops::MaxPool1D<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::MaxPool1D<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::MAX_POOL_2D:
  {
    g_ptr->template AddNode<ops::MaxPool2D<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::MaxPool2D<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::TRANSPOSE:
  {
    g_ptr->template AddNode<ops::Transpose<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::Transpose<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::RESHAPE:
  {
    g_ptr->template AddNode<ops::Reshape<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::ops::Reshape<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::LAYER_FULLY_CONNECTED:
  {
    g_ptr->template AddNode<layers::FullyConnected<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::layers::FullyConnected<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::LAYER_CONVOLUTION_1D:
  {
    g_ptr->template AddNode<layers::Convolution1D<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::layers::Convolution1D<TensorType>::SPType>(
            saved_node)));
    break;
  }
  case OpType::LAYER_CONVOLUTION_2D:
  {
    g_ptr->template AddNode<layers::Convolution2D<TensorType>>(
        node_name, inputs,
        *(std::dynamic_pointer_cast<typename fetch::ml::layers::Convolution2D<TensorType>::SPType>(
            saved_node)));
    break;
  }
  default:
  {
    throw std::runtime_error("Unknown type for Serialization");
  }
  }
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

    fetch::ml::utilities::AddToGraph<TensorType>(gsp->nodes[name].OP_DESCRIPTOR, ret,
                                                 gsp->nodes[name], name, inputs);
  }

  return *ret;
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
