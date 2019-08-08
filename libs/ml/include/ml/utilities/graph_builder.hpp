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
#include "ml/layers/self_attention_encoder.hpp"
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
#include "ml/ops/switch.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/weights.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace utilities {

template <typename T>
void BuildNodeAndInsertTrainables(NodeSaveableParams<T> const &nsp, std::string const &name,
                                  std::shared_ptr<Graph<T>> g);

template <typename T>
void BuildGraph(GraphSaveableParams<T> const &sp, std::shared_ptr<Graph<T>> ret)
{
  // loop through connections adding nodes
  for (auto &node : sp.nodes)
  {
    std::string name = node.first;
    BuildNodeAndInsertTrainables(*(std::dynamic_pointer_cast<NodeSaveableParams<T>>(node.second)),
                                 name, ret);
  }
  ret->SetGraphSaveableParams(sp);
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

template <typename T, typename OperationType>
std::shared_ptr<OperationType> BuildLayer(std::shared_ptr<SaveableParamsInterface> op_save_params)
{
  auto sp  = *(std::dynamic_pointer_cast<typename OperationType::SPType>(op_save_params));
  auto ret = std::make_shared<OperationType>();
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
void BuildNodeAndInsertTrainables(NodeSaveableParams<T> const &nsp, std::string const &name,
                                  std::shared_ptr<Graph<T>> g)
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
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Abs<T>>(name, node);
    break;
  }
  case ops::Add<T>::OpCode():
  {
    op_ptr = GetOp<ops::Add<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Add<T>>(name, node);
    break;
  }
  case ops::Concatenate<T>::OpCode():
  {
    op_ptr = GetOp<ops::Concatenate<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Concatenate<T>>(name, node);
    break;
  }
  case ops::Convolution1D<T>::OpCode():
  {
    op_ptr = GetOp<ops::Convolution1D<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Convolution1D<T>>(name, node);
    break;
  }
  case ops::Convolution2D<T>::OpCode():
  {
    op_ptr = GetOp<ops::Convolution2D<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Convolution2D<T>>(name, node);
    break;
  }
  case ops::CrossEntropyLoss<T>::OpCode():
  {
    op_ptr = GetOp<ops::CrossEntropyLoss<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::CrossEntropyLoss<T>>(name, node);
    break;
  }
  case ops::Divide<T>::OpCode():
  {
    op_ptr = GetOp<ops::Divide<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Divide<T>>(name, node);
    break;
  }
  case ops::Dropout<T>::OpCode():
  {
    op_ptr = GetOp<ops::Dropout<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Dropout<T>>(name, node);
    break;
  }
  case ops::Elu<T>::OpCode():
  {
    op_ptr = GetOp<ops::Elu<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Elu<T>>(name, node);
    break;
  }
  case ops::Embeddings<T>::OpCode():
  {
    op_ptr = GetOp<ops::Embeddings<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Embeddings<T>>(name, node);
    break;
  }
  case ops::Exp<T>::OpCode():
  {
    op_ptr = GetOp<ops::Exp<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Exp<T>>(name, node);
    break;
  }
  case ops::Flatten<T>::OpCode():
  {
    op_ptr = GetOp<ops::Flatten<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Flatten<T>>(name, node);
    break;
  }
  case ops::LayerNorm<T>::OpCode():
  {
    op_ptr = GetOp<ops::LayerNorm<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::LayerNorm<T>>(name, node);
    break;
  }
  case ops::LeakyRelu<T>::OpCode():
  {
    op_ptr = GetOp<ops::LeakyRelu<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::LeakyRelu<T>>(name, node);
    break;
  }
  case ops::LeakyReluOp<T>::OpCode():
  {
    op_ptr = GetOp<ops::LeakyReluOp<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::LeakyReluOp<T>>(name, node);
    break;
  }
  case ops::Log<T>::OpCode():
  {
    op_ptr = GetOp<ops::Log<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Log<T>>(name, node);
    break;
  }
  case ops::LogSigmoid<T>::OpCode():
  {
    op_ptr = GetOp<ops::LogSigmoid<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::LogSigmoid<T>>(name, node);
    break;
  }
  case ops::LogSoftmax<T>::OpCode():
  {
    op_ptr = GetOp<ops::LogSoftmax<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::LogSoftmax<T>>(name, node);
    break;
  }
  case ops::MatrixMultiply<T>::OpCode():
  {
    op_ptr = GetOp<ops::MatrixMultiply<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::MatrixMultiply<T>>(name, node);
    break;
  }
  case ops::MeanSquareErrorLoss<T>::OpCode():
  {
    op_ptr = GetOp<ops::MeanSquareErrorLoss<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::MeanSquareErrorLoss<T>>(name, node);
    break;
  }
  case ops::MaxPool1D<T>::OpCode():
  {
    op_ptr = GetOp<ops::MaxPool1D<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::MaxPool1D<T>>(name, node);
    break;
  }
  case ops::MaxPool2D<T>::OpCode():
  {
    op_ptr = GetOp<ops::MaxPool2D<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::MaxPool2D<T>>(name, node);
    break;
  }
  case ops::Maximum<T>::OpCode():
  {
    op_ptr = GetOp<ops::Maximum<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Maximum<T>>(name, node);
    break;
  }
  case ops::Multiply<T>::OpCode():
  {
    op_ptr = GetOp<ops::Multiply<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Multiply<T>>(name, node);
    break;
  }
  case ops::PlaceHolder<T>::OpCode():
  {
    op_ptr = GetOp<ops::PlaceHolder<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::PlaceHolder<T>>(name, node);
    break;
  }
  case ops::RandomisedRelu<T>::OpCode():
  {
    op_ptr = GetOp<ops::RandomisedRelu<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::RandomisedRelu<T>>(name, node);
    break;
  }
  case ops::Relu<T>::OpCode():
  {
    op_ptr = GetOp<ops::Relu<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Relu<T>>(name, node);
    break;
  }
  case ops::Reshape<T>::OpCode():
  {
    op_ptr = GetOp<ops::Reshape<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Reshape<T>>(name, node);
    break;
  }
  case ops::Sigmoid<T>::OpCode():
  {
    op_ptr = GetOp<ops::Sigmoid<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Sigmoid<T>>(name, node);
    break;
  }
  case ops::Softmax<T>::OpCode():
  {
    op_ptr = GetOp<ops::Softmax<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Softmax<T>>(name, node);
    break;
  }
  case ops::SoftmaxCrossEntropyLoss<T>::OpCode():
  {
    op_ptr = GetOp<ops::SoftmaxCrossEntropyLoss<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::SoftmaxCrossEntropyLoss<T>>(name, node);
    break;
  }
  case ops::Sqrt<T>::OpCode():
  {
    op_ptr = GetOp<ops::Sqrt<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Sqrt<T>>(name, node);
    break;
  }
  case ops::Subtract<T>::OpCode():
  {
    op_ptr = GetOp<ops::Subtract<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Subtract<T>>(name, node);
    break;
  }
  case ops::Switch<T>::OpCode():
  {
    op_ptr = GetOp<ops::Switch<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Switch<T>>(name, node);
    break;
  }
  case ops::TanH<T>::OpCode():
  {
    op_ptr = GetOp<ops::TanH<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::TanH<T>>(name, node);
    break;
  }
  case ops::Transpose<T>::OpCode():
  {
    op_ptr = GetOp<ops::Transpose<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Transpose<T>>(name, node);
    break;
  }
  case ops::Weights<T>::OpCode():
  {
    op_ptr = GetOp<ops::Weights<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<ops::Weights<T>>(name, node);
    break;
  }
  case OpType::LAYER_CONVOLUTION_1D:
  {
    op_ptr = BuildLayer<T, layers::Convolution1D<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<layers::Convolution1D<T>>(name, node);
    break;
  }
  case OpType::LAYER_CONVOLUTION_2D:
  {
    op_ptr = BuildLayer<T, layers::Convolution2D<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<layers::Convolution2D<T>>(name, node);
    break;
  }
  case OpType::LAYER_FULLY_CONNECTED:
  {
    op_ptr = BuildLayer<T, layers::FullyConnected<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<layers::FullyConnected<T>>(name, node);
    break;
  }
  case OpType::LAYER_LAYER_NORM:
  {
    op_ptr = BuildLayer<T, layers::LayerNorm<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<layers::LayerNorm<T>>(name, node);
    break;
  }
  case OpType::LAYER_MULTI_HEAD_ATTENTION:
  {
    op_ptr = BuildLayer<T, layers::MultiheadAttention<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<layers::MultiheadAttention<T>>(name, node);
    break;
  }
  case OpType::LAYER_PRELU:
  {
    op_ptr = BuildLayer<T, layers::PRelu<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<layers::PRelu<T>>(name, node);
    break;
  }
  case OpType::LAYER_SCALED_DOT_PRODUCT_ATTENTION:
  {
    op_ptr = BuildLayer<T, layers::ScaledDotProductAttention<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<layers::ScaledDotProductAttention<T>>(name, node);
    break;
  }
  case OpType::LAYER_SELF_ATTENTION_ENCODER:
  {
    op_ptr = BuildLayer<T, layers::SelfAttentionEncoder<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<layers::SelfAttentionEncoder<T>>(name, node);
    break;
  }
  case OpType::LAYER_SKIP_GRAM:
  {
    op_ptr = BuildLayer<T, layers::SkipGram<T>>(nsp.op_save_params);
    node->SetNodeSaveableParams(nsp, op_ptr);
    g->template AddTrainable<layers::SkipGram<T>>(name, node);
    break;
  }
  default:
    throw std::runtime_error("unknown node type");
  }

  if (!(g->InsertNode(name, node)))
  {
    throw std::runtime_error("BuildGraph unable to insert node");
  }
}

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
