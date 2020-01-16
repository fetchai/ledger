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

#include "ml/utilities/graph_builder.hpp"

#include "ml/ops/metrics/categorical_accuracy.hpp"

#include "ml/layers/PRelu.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/multihead_attention.hpp"
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/layers/skip_gram.hpp"

#include "ml/meta/ml_type_traits.hpp"

#include "ml/ops/abs.hpp"
#include "ml/ops/activations/dropout.hpp"
#include "ml/ops/activations/elu.hpp"
#include "ml/ops/activations/gelu.hpp"
#include "ml/ops/activations/leaky_relu.hpp"
#include "ml/ops/activations/logsigmoid.hpp"
#include "ml/ops/activations/logsoftmax.hpp"
#include "ml/ops/activations/randomised_relu.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/activations/sigmoid.hpp"
#include "ml/ops/activations/softmax.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/avg_pool_1d.hpp"
#include "ml/ops/avg_pool_2d.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/exp.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/log.hpp"
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/loss_functions/softmax_cross_entropy_loss.hpp"
#include "ml/ops/mask_fill.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/max_pool.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"
#include "ml/ops/maximum.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/one_hot.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/reduce_mean.hpp"
#include "ml/ops/reshape.hpp"
#include "ml/ops/slice.hpp"
#include "ml/ops/sqrt.hpp"
#include "ml/ops/squeeze.hpp"
#include "ml/ops/strided_slice.hpp"
#include "ml/ops/subtract.hpp"
#include "ml/ops/switch.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/top_k.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/weights.hpp"
#include "ml/saveparams/saveable_params.hpp"

namespace fetch {
namespace ml {
namespace utilities {

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
    break;
  }
  case ops::Add<T>::OpCode():
  {
    op_ptr = GetOp<ops::Add<T>>(nsp.op_save_params);
    break;
  }
  case ops::Concatenate<T>::OpCode():
  {
    op_ptr = GetOp<ops::Concatenate<T>>(nsp.op_save_params);
    break;
  }
  case ops::Constant<T>::OpCode():
  {
    op_ptr = GetOp<ops::Constant<T>>(nsp.op_save_params);
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
  case ops::CrossEntropyLoss<T>::OpCode():
  {
    op_ptr = GetOp<ops::CrossEntropyLoss<T>>(nsp.op_save_params);
    break;
  }
  case ops::Divide<T>::OpCode():
  {
    op_ptr = GetOp<ops::Divide<T>>(nsp.op_save_params);
    break;
  }
  case ops::Dropout<T>::OpCode():
  {
    op_ptr = GetOp<ops::Dropout<T>>(nsp.op_save_params);
    break;
  }
  case ops::Elu<T>::OpCode():
  {
    op_ptr = GetOp<ops::Elu<T>>(nsp.op_save_params);
    break;
  }
  case ops::Embeddings<T>::OpCode():
  {
    op_ptr = GetOp<ops::Embeddings<T>>(nsp.op_save_params);
    break;
  }
  case ops::Exp<T>::OpCode():
  {
    op_ptr = GetOp<ops::Exp<T>>(nsp.op_save_params);
    break;
  }
  case ops::Flatten<T>::OpCode():
  {
    op_ptr = GetOp<ops::Flatten<T>>(nsp.op_save_params);
    break;
  }
  case ops::Gelu<T>::OpCode():
  {
    op_ptr = GetOp<ops::Gelu<T>>(nsp.op_save_params);
    break;
  }
  case ops::LayerNorm<T>::OpCode():
  {
    op_ptr = GetOp<ops::LayerNorm<T>>(nsp.op_save_params);
    break;
  }
  case ops::LeakyRelu<T>::OpCode():
  {
    op_ptr = GetOp<ops::LeakyRelu<T>>(nsp.op_save_params);
    break;
  }
  case ops::PReluOp<T>::OpCode():
  {
    op_ptr = GetOp<ops::PReluOp<T>>(nsp.op_save_params);
    break;
  }
  case ops::Log<T>::OpCode():
  {
    op_ptr = GetOp<ops::Log<T>>(nsp.op_save_params);
    break;
  }
  case ops::LogSigmoid<T>::OpCode():
  {
    op_ptr = GetOp<ops::LogSigmoid<T>>(nsp.op_save_params);
    break;
  }
  case ops::LogSoftmax<T>::OpCode():
  {
    op_ptr = GetOp<ops::LogSoftmax<T>>(nsp.op_save_params);
    break;
  }
  case ops::MatrixMultiply<T>::OpCode():
  {
    op_ptr = GetOp<ops::MatrixMultiply<T>>(nsp.op_save_params);
    break;
  }
  case ops::MeanSquareErrorLoss<T>::OpCode():
  {
    op_ptr = GetOp<ops::MeanSquareErrorLoss<T>>(nsp.op_save_params);
    break;
  }
  case ops::MaskFill<T>::OpCode():
  {
    op_ptr = GetOp<ops::MaskFill<T>>(nsp.op_save_params);
    break;
  }
  case ops::MaxPool1D<T>::OpCode():
  {
    op_ptr = GetOp<ops::MaxPool1D<T>>(nsp.op_save_params);
    break;
  }
  case ops::MaxPool2D<T>::OpCode():
  {
    op_ptr = GetOp<ops::MaxPool2D<T>>(nsp.op_save_params);
    break;
  }
  case ops::MaxPool<T>::OpCode():
  {
    op_ptr = GetOp<ops::MaxPool<T>>(nsp.op_save_params);
    break;
  }
  case ops::AvgPool1D<T>::OpCode():
  {
    op_ptr = GetOp<ops::AvgPool1D<T>>(nsp.op_save_params);
    break;
  }
  case ops::AvgPool2D<T>::OpCode():
  {
    op_ptr = GetOp<ops::AvgPool2D<T>>(nsp.op_save_params);
    break;
  }
  case ops::Maximum<T>::OpCode():
  {
    op_ptr = GetOp<ops::Maximum<T>>(nsp.op_save_params);
    break;
  }
  case ops::Multiply<T>::OpCode():
  {
    op_ptr = GetOp<ops::Multiply<T>>(nsp.op_save_params);
    break;
  }
  case ops::PlaceHolder<T>::OpCode():
  {
    op_ptr = GetOp<ops::PlaceHolder<T>>(nsp.op_save_params);
    break;
  }
  case ops::RandomisedRelu<T>::OpCode():
  {
    op_ptr = GetOp<ops::RandomisedRelu<T>>(nsp.op_save_params);
    break;
  }
  case ops::Relu<T>::OpCode():
  {
    op_ptr = GetOp<ops::Relu<T>>(nsp.op_save_params);
    break;
  }
  case ops::Reshape<T>::OpCode():
  {
    op_ptr = GetOp<ops::Reshape<T>>(nsp.op_save_params);
    break;
  }
  case ops::Sigmoid<T>::OpCode():
  {
    op_ptr = GetOp<ops::Sigmoid<T>>(nsp.op_save_params);
    break;
  }
  case ops::Slice<T>::OpCode():
  {
    op_ptr = GetOp<ops::Slice<T>>(nsp.op_save_params);
    break;
  }
  case ops::StridedSlice<T>::OpCode():
  {
    op_ptr = GetOp<ops::StridedSlice<T>>(nsp.op_save_params);
    break;
  }
  case ops::ReduceMean<T>::OpCode():
  {
    op_ptr = GetOp<ops::ReduceMean<T>>(nsp.op_save_params);
    break;
  }
  case ops::Softmax<T>::OpCode():
  {
    op_ptr = GetOp<ops::Softmax<T>>(nsp.op_save_params);
    break;
  }
  case ops::SoftmaxCrossEntropyLoss<T>::OpCode():
  {
    op_ptr = GetOp<ops::SoftmaxCrossEntropyLoss<T>>(nsp.op_save_params);
    break;
  }
  case ops::Sqrt<T>::OpCode():
  {
    op_ptr = GetOp<ops::Sqrt<T>>(nsp.op_save_params);
    break;
  }
  case ops::Subtract<T>::OpCode():
  {
    op_ptr = GetOp<ops::Subtract<T>>(nsp.op_save_params);
    break;
  }
  case ops::Switch<T>::OpCode():
  {
    op_ptr = GetOp<ops::Switch<T>>(nsp.op_save_params);
    break;
  }
  case ops::TanH<T>::OpCode():
  {
    op_ptr = GetOp<ops::TanH<T>>(nsp.op_save_params);
    break;
  }
  case ops::Transpose<T>::OpCode():
  {
    op_ptr = GetOp<ops::Transpose<T>>(nsp.op_save_params);
    break;
  }
  case ops::OneHot<T>::OpCode():
  {
    op_ptr = GetOp<ops::OneHot<T>>(nsp.op_save_params);
    break;
  }
  case ops::TopK<T>::OpCode():
  {
    op_ptr = GetOp<ops::TopK<T>>(nsp.op_save_params);
    break;
  }
  case ops::Squeeze<T>::OpCode():
  {
    op_ptr = GetOp<ops::Squeeze<T>>(nsp.op_save_params);
    break;
  }
  case ops::Weights<T>::OpCode():
  {
    op_ptr = GetOp<ops::Weights<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_CONVOLUTION_1D:
  {
    op_ptr = BuildLayer<T, layers::Convolution1D<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_CONVOLUTION_2D:
  {
    op_ptr = BuildLayer<T, layers::Convolution2D<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_FULLY_CONNECTED:
  {
    op_ptr = BuildLayer<T, layers::FullyConnected<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_LAYER_NORM:
  {
    op_ptr = BuildLayer<T, layers::LayerNorm<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_MULTI_HEAD_ATTENTION:
  {
    op_ptr = BuildLayer<T, layers::MultiheadAttention<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_PRELU:
  {
    op_ptr = BuildLayer<T, layers::PRelu<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_SCALED_DOT_PRODUCT_ATTENTION:
  {
    op_ptr = BuildLayer<T, layers::ScaledDotProductAttention<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_SELF_ATTENTION_ENCODER:
  {
    op_ptr = BuildLayer<T, layers::SelfAttentionEncoder<T>>(nsp.op_save_params);
    break;
  }
  case OpType::LAYER_SKIP_GRAM:
  {
    op_ptr = BuildLayer<T, layers::SkipGram<T>>(nsp.op_save_params);
    break;
  }
  case OpType::METRIC_CATEGORICAL_ACCURACY:
  {
    op_ptr = GetOp<ops::CategoricalAccuracy<T>>(nsp.op_save_params);
    break;
  }
  case OpType::GRAPH:
  case OpType::NONE:
  case OpType::SUBGRAPH:
  case OpType::OP_VARIABLE:
  case OpType::OP_DATAHOLDER:
  {
    throw ml::exceptions::NotImplemented();
  }
  default:
    throw ml::exceptions::NotImplemented("unknown node type");
  }

  node->SetNodeSaveableParams(nsp, op_ptr);
  g->AddTrainable(node, name);

  if (!(g->InsertNode(name, node)))
  {
    throw ml::exceptions::InvalidMode("BuildGraph unable to insert node");
  }
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

template void BuildNodeAndInsertTrainables<math::Tensor<int8_t>>(
    NodeSaveableParams<math::Tensor<int8_t>> const &, std::string const &,
    std::shared_ptr<Graph<math::Tensor<int8_t>>>);
template void BuildNodeAndInsertTrainables<math::Tensor<int16_t>>(
    NodeSaveableParams<math::Tensor<int16_t>> const &, std::string const &,
    std::shared_ptr<Graph<math::Tensor<int16_t>>>);
template void BuildNodeAndInsertTrainables<math::Tensor<int32_t>>(
    NodeSaveableParams<math::Tensor<int32_t>> const &, std::string const &,
    std::shared_ptr<Graph<math::Tensor<int32_t>>>);
template void BuildNodeAndInsertTrainables<math::Tensor<int64_t>>(
    NodeSaveableParams<math::Tensor<int64_t>> const &, std::string const &,
    std::shared_ptr<Graph<math::Tensor<int64_t>>>);
template void BuildNodeAndInsertTrainables<math::Tensor<float>>(
    NodeSaveableParams<math::Tensor<float>> const &, std::string const &,
    std::shared_ptr<Graph<math::Tensor<float>>>);
template void BuildNodeAndInsertTrainables<math::Tensor<double>>(
    NodeSaveableParams<math::Tensor<double>> const &, std::string const &,
    std::shared_ptr<Graph<math::Tensor<double>>>);
template void BuildNodeAndInsertTrainables<math::Tensor<fixed_point::fp32_t>>(
    NodeSaveableParams<math::Tensor<fixed_point::fp32_t>> const &, std::string const &,
    std::shared_ptr<Graph<math::Tensor<fixed_point::fp32_t>>>);
template void BuildNodeAndInsertTrainables<math::Tensor<fixed_point::fp64_t>>(
    NodeSaveableParams<math::Tensor<fixed_point::fp64_t>> const &, std::string const &,
    std::shared_ptr<Graph<math::Tensor<fixed_point::fp64_t>>>);
template void BuildNodeAndInsertTrainables<math::Tensor<fixed_point::fp128_t>>(
    NodeSaveableParams<math::Tensor<fixed_point::fp128_t>> const &, std::string const &,
    std::shared_ptr<Graph<math::Tensor<fixed_point::fp128_t>>>);

}  // namespace utilities
}  // namespace ml
}  // namespace fetch
