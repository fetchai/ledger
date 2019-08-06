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

#include "ml/core/graph.hpp"
#include "ml/layers/PRelu.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/layers_declaration.hpp"
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

template <typename T>
std::shared_ptr<Node<T>> BuildNode(NodeSaveableParams<T> const &nsp);

template <typename T>
void BuildGraph(GraphSaveableParams<T> const &sp, std::shared_ptr<Graph<T>> ret)
{
  std::unordered_map<std::string, std::shared_ptr<Node<T>>> nodes_map;

  // loop through connections adding nodes
  for (auto &node : sp.nodes)
  {
    std::string name = node.first;
    auto        node_ptr =
        BuildNode(*(std::dynamic_pointer_cast<NodeSaveableParams<T>>(node.second)));

    bool success = ret->InsertNode(name, node_ptr);
    assert(success);

    nodes_map[name] = node_ptr;
  }

  // assign inputs and outputs to the nodes
  for (auto &node : sp.connections) {
    Graph<T>::LinkNodesInGraph(node.first, node.second, nodes_map);
  }

  // add to trainable
  // todo: recreate trainables

}

template <typename T>
void BuildSubGraph(SubGraphSaveableParams<T> const &sgsp, std::shared_ptr<SubGraph<T>> ret)
{
  BuildGraph<T>(sgsp, ret);

  for (auto const &name : sgsp.input_node_names)
  {
    ret->AddInputNode(name);
  }
  ret->SetOutputNode(sgsp.output_node_name);
}

template <typename T>
std::shared_ptr<layers::Convolution1D<T>> BuildLayerConvolution1D(
    ConvolutionLayer1DSaveableParams<T> const &sp)
{
  auto ret = std::make_shared<layers::Convolution1D<T>>();
  BuildSubGraph<T>(sp, ret);
  ret->SetOpSaveableParams(sp);

  return ret;
}

template <typename T>
std::shared_ptr<layers::Convolution2D<T>> BuildLayerConvolution2D(
    ConvolutionLayer2DSaveableParams<T> const &sp)
{
  auto ret = std::make_shared<layers::Convolution2D<T>>();
  BuildSubGraph<T>(sp, ret);
  ret->SetOpSaveableParams(sp);

  return ret;
}

template <typename T>
std::shared_ptr<layers::PRelu<T>> BuildLayerPrelu(PReluSaveableParams<T> const &sp)
{
  auto ret = std::make_shared<layers::PRelu<T>>();
  BuildSubGraph<T>(sp, ret);
  ret->SetOpSaveableParams(sp);

  return ret;
}

template <typename T>
std::shared_ptr<layers::SelfAttention<T>> BuildLayerSelfAttention(
    SelfAttentionSaveableParams<T> const &sp)
{
  auto ret = std::make_shared<layers::SelfAttention<T>>();
  BuildSubGraph<T>(sp, ret);
  ret->SetOpSaveableParams(sp);

  return ret;
}

template <typename T>
std::shared_ptr<layers::FullyConnected<T>> BuildLayerFullyConnected(FullyConnectedSaveableParams<T> const &sp)
{
  auto ret = std::make_shared<layers::FullyConnected<T>>();
  BuildSubGraph<T>(sp, ret);
  ret->SetOpSaveableParams(sp);

  return ret;
}

namespace {
template <class OperationType>
std::shared_ptr<OperationType> GetOp(std::shared_ptr<SaveableParamsInterface> op_save_params)
{
  return std::make_shared<OperationType>(
      *(std::dynamic_pointer_cast<typename OperationType::SPType>(op_save_params)));
}
}  // namespace

template <typename T>
std::shared_ptr<Node<T>> BuildNode(NodeSaveableParams<T> const &nsp)
{
  auto node = std::make_shared<Node<T>>();
  // construct the concrete op
  // if the op is a layer, invoke the layer Build method

  std::shared_ptr<ops::Ops<T>> op_ptr;
  switch (nsp.operation_type)
  {
  case ops::Abs<T>::OpCode():
  {
    op_ptr = GetOp<ops::Abs<T>>(nsp.op_save_params);
    break;
  }
  case ops::Add<T>::OpCode():
  {
    op_ptr = GetOp<ops::Add<T>>(nsp.op_save_params);
    break;
  }
  case ops::Convolution1D<T>::OpCode():
  {
    op_ptr = GetOp<ops::Convolution1D<T>>(nsp.op_save_params);
    break;
  }
  case ops::Convolution2D<T>::OpCode():
  {
    op_ptr = GetOp<ops::Convolution2D<T>>(nsp.op_save_params);
    break;
  }
  case ops::Flatten<T>::OpCode():
  {
    op_ptr = GetOp<ops::Flatten<T>>(nsp.op_save_params);
    break;
  }
  case ops::LeakyReluOp<T>::OpCode():
  {
    op_ptr = GetOp<ops::LeakyReluOp<T>>(nsp.op_save_params);
    break;
  }
  case ops::MatrixMultiply<T>::OpCode():
  {
    op_ptr = GetOp<ops::MatrixMultiply<T>>(nsp.op_save_params);
    break;
  }
  case ops::PlaceHolder<T>::OpCode():
  {
    op_ptr = GetOp<ops::PlaceHolder<T>>(nsp.op_save_params);
    break;
  }
  case ops::Relu<T>::OpCode():
  {
    op_ptr = GetOp<ops::Relu<T>>(nsp.op_save_params);
    break;
  }
  case ops::Softmax<T>::OpCode():
  {
    op_ptr = GetOp<ops::Softmax<T>>(nsp.op_save_params);
    break;
  }
  case ops::Transpose<T>::OpCode():
  {
    op_ptr = GetOp<ops::Transpose<T>>(nsp.op_save_params);
    break;
  }
  case ops::Weights<T>::OpCode():
  {
    op_ptr = GetOp<ops::Weights<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_CONVOLUTION_1D:
  {
    op_ptr = BuildLayerConvolution1D(
        *(std::dynamic_pointer_cast<ConvolutionLayer1DSaveableParams<T>>(nsp.op_save_params)));
    break;
  }
  case OpType::LAYER_CONVOLUTION_2D:
  {
    op_ptr = BuildLayerConvolution2D(
        *(std::dynamic_pointer_cast<ConvolutionLayer2DSaveableParams<T>>(nsp.op_save_params)));
    break;
  }
  case OpType::LAYER_FULLY_CONNECTED:
  {
    op_ptr = BuildLayerFullyConnected(*(std::dynamic_pointer_cast<FullyConnectedSaveableParams<T>>(nsp.op_save_params)));
    break;
  }
  case OpType::LAYER_PRELU:
  {
    op_ptr =
        BuildLayerPrelu(*(std::dynamic_pointer_cast<PReluSaveableParams<T>>(nsp.op_save_params)));
    break;
  }
  case OpType::LAYER_SELF_ATTENTION:
  {
    op_ptr = BuildLayerSelfAttention(
        *(std::dynamic_pointer_cast<SelfAttentionSaveableParams<T>>(nsp.op_save_params)));
    break;
  }
  default:
    throw std::runtime_error("unknown node type");
  }

  node->SetNodeSaveableParams(nsp, op_ptr);
  return node;
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
