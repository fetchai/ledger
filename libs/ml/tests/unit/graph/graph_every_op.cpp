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

#include "math/tensor.hpp"
#include "ml/core/graph.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

// ordinary ops
#include "ml/ops/abs.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/avg_pool_1d.hpp"
#include "ml/ops/avg_pool_2d.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/constant.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/divide.hpp"
#include "ml/ops/embeddings.hpp"
#include "ml/ops/exp.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/layer_norm.hpp"
#include "ml/ops/log.hpp"
#include "ml/ops/mask_fill.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/max_pool.hpp"
#include "ml/ops/max_pool_1d.hpp"
#include "ml/ops/max_pool_2d.hpp"
#include "ml/ops/maximum.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/one_hot.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/prelu_op.hpp"
#include "ml/ops/reduce_mean.hpp"
#include "ml/ops/slice.hpp"
#include "ml/ops/sqrt.hpp"
#include "ml/ops/squeeze.hpp"
#include "ml/ops/switch.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/ops/top_k.hpp"
#include "ml/ops/transpose.hpp"
#include "ml/ops/weights.hpp"

// activations
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

// loss functions
#include "ml/ops/loss_functions/cross_entropy_loss.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/loss_functions/softmax_cross_entropy_loss.hpp"

// layers
#include "ml/layers/PRelu.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/multihead_attention.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/layers/skip_gram.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class GraphTest : public ::testing::Test
{
};

TYPED_TEST_CASE(GraphTest, math::test::TensorFloatingTypes);

TYPED_TEST(GraphTest, graph_rebuild_every_op)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  // Create graph
  std::string                 name = "Graph";
  fetch::ml::Graph<TypeParam> g;
  std::string                 input_1 =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input_1", {});
  std::string input_2 =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input_2", {});

  // ordinary ops
  std::string abs = g.template AddNode<fetch::ml::ops::Abs<TensorType>>(name + "_Abs", {input_1});
  std::string add =
      g.template AddNode<fetch::ml::ops::Add<TensorType>>(name + "_Add", {input_1, input_2});
  std::string avg1 = g.template AddNode<fetch::ml::ops::AvgPool1D<TensorType>>(name + "_AvgPool1D",
                                                                               {input_1}, 1, 1);
  std::string avg2 = g.template AddNode<fetch::ml::ops::AvgPool2D<TensorType>>(name + "_AvgPool2D",
                                                                               {input_1}, 1, 1);
  std::string concat = g.template AddNode<fetch::ml::ops::Concatenate<TensorType>>(
      name + "_Concatenate", {input_1, input_2}, 0);
  std::string constant =
      g.template AddNode<fetch::ml::ops::Constant<TensorType>>(name + "_Constant", {input_1});
  std::string conv1d =
      g.template AddNode<fetch::ml::ops::Convolution1D<TensorType>>(name + "_Conv1D", {input_1});
  std::string conv2d =
      g.template AddNode<fetch::ml::ops::Convolution2D<TensorType>>(name + "_Conv2D", {input_2});
  std::string divide =
      g.template AddNode<fetch::ml::ops::Divide<TensorType>>(name + "_Divide", {input_1});
  std::string embed = g.template AddNode<fetch::ml::ops::Embeddings<TensorType>>(
      name + "_Embeddings", {input_1}, 1, 3);
  std::string exp = g.template AddNode<fetch::ml::ops::Exp<TensorType>>(name + "_Exp", {input_1});
  std::string flatten =
      g.template AddNode<fetch::ml::ops::Flatten<TensorType>>(name + "_Flatten", {input_1});
  std::string layernorm =
      g.template AddNode<fetch::ml::ops::LayerNorm<TensorType>>(name + "LayerNorm", {input_1});
  std::string log = g.template AddNode<fetch::ml::ops::Log<TensorType>>(name + "_Log", {input_1});
  std::string maskfill = g.template AddNode<fetch::ml::ops::MaskFill<TensorType>>(
      name + "_MaskFill", {input_1, input_1}, DataType(0));
  std::string matmul = g.template AddNode<fetch::ml::ops::MatrixMultiply<TensorType>>(
      name + "_MatrixMultiply", {input_1});
  std::string maxpool =
      g.template AddNode<fetch::ml::ops::MaxPool<TensorType>>(name + "_MaxPool", {input_1}, 1, 1);
  std::string maxpool1d = g.template AddNode<fetch::ml::ops::MaxPool1D<TensorType>>(
      name + "_MaxPool1D", {input_1}, 1, 1);
  std::string maxpool2d = g.template AddNode<fetch::ml::ops::MaxPool2D<TensorType>>(
      name + "_MaxPool2D", {input_1}, 1, 1);
  std::string maximum =
      g.template AddNode<fetch::ml::ops::Maximum<TensorType>>(name + "_Maximum", {input_1});
  std::string multiply =
      g.template AddNode<fetch::ml::ops::Multiply<TensorType>>(name + "_Multiply", {input_1});
  std::string onehot =
      g.template AddNode<fetch::ml::ops::OneHot<TensorType>>(name + "_OneHot", {input_1}, 2);
  std::string placeholder =
      g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_PlaceHolder", {input_1});
  std::string reducemean = g.template AddNode<fetch::ml::ops::ReduceMean<TensorType>>(
      name + "_ReduceMean", {input_1}, 0);
  std::string slice =
      g.template AddNode<fetch::ml::ops::Slice<TensorType>>(name + "_Slice", {input_1}, 0, 0);
  std::string sqrt =
      g.template AddNode<fetch::ml::ops::Sqrt<TensorType>>(name + "_Sqrt", {input_1});
  std::string squeeze =
      g.template AddNode<fetch::ml::ops::Squeeze<TensorType>>(name + "_Squeeze", {input_1});
  std::string switchop =
      g.template AddNode<fetch::ml::ops::Switch<TensorType>>(name + "_Switch", {input_1});
  std::string tanh =
      g.template AddNode<fetch::ml::ops::TanH<TensorType>>(name + "_TanH", {input_1});
  std::string transpose =
      g.template AddNode<fetch::ml::ops::Transpose<TensorType>>(name + "_Transpose", {input_1});
  std::string topk =
      g.template AddNode<fetch::ml::ops::TopK<TensorType>>(name + "_TopK", {input_1}, 2);
  std::string weights =
      g.template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights", {input_1});

  // activations
  std::string dropout =
      g.template AddNode<fetch::ml::ops::Dropout<TensorType>>(name + "_Dropout", {input_1});
  std::string elu = g.template AddNode<fetch::ml::ops::Elu<TensorType>>(name + "_Elu", {input_1});
  std::string gelu =
      g.template AddNode<fetch::ml::ops::Gelu<TensorType>>(name + "_Gelu", {input_1});
  std::string leakyrelu =
      g.template AddNode<fetch::ml::ops::LeakyRelu<TensorType>>(name + "_LeakyRelu", {input_1});
  std::string logsigmoid =
      g.template AddNode<fetch::ml::ops::LogSigmoid<TensorType>>(name + "_LogSigmoid", {input_1});
  std::string logsoftmax =
      g.template AddNode<fetch::ml::ops::LogSoftmax<TensorType>>(name + "_LogSoftmax", {input_1});
  std::string randomisedrelu = g.template AddNode<fetch::ml::ops::RandomisedRelu<TensorType>>(
      name + "_RandomisedRelu", {input_1});
  std::string relu =
      g.template AddNode<fetch::ml::ops::Relu<TensorType>>(name + "_Relu", {input_1});
  std::string sigmoid =
      g.template AddNode<fetch::ml::ops::Sigmoid<TensorType>>(name + "_Sigmoid", {input_1});
  std::string softmax =
      g.template AddNode<fetch::ml::ops::Softmax<TensorType>>(name + "_Softmax", {input_1});

  // Loss functions
  std::string cel =
      g.template AddNode<fetch::ml::ops::CrossEntropyLoss<TensorType>>(name + "_CEL", {input_1});
  std::string mse =
      g.template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(name + "_MSE", {input_1});
  std::string scel = g.template AddNode<fetch::ml::ops::SoftmaxCrossEntropyLoss<TensorType>>(
      name + "_SCEL", {input_1});

  // Layers
  std::string layer_layernorm =
      g.template AddNode<fetch::ml::layers::LayerNorm<TensorType>>(name + "_LayerNorm", {input_1});
  std::string layer_conv1d = g.template AddNode<fetch::ml::layers::Convolution1D<TensorType>>(
      name + "_Convolution1D", {input_1});
  std::string layer_conv2d = g.template AddNode<fetch::ml::layers::Convolution2D<TensorType>>(
      name + "__Convolution2D", {input_1});
  std::string layer_fc1 =
      g.template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(name + "_FC1", {input_1});
  std::string layer_mh = g.template AddNode<fetch::ml::layers::MultiheadAttention<TensorType>>(
      name + "_MultiHead", {input_1});
  std::string layer_prelu =
      g.template AddNode<fetch::ml::layers::PRelu<TensorType>>(name + "_PRELU", {input_1});
  std::string layer_scaleddotproductattention =
      g.template AddNode<fetch::ml::layers::ScaledDotProductAttention<TensorType>>(
          name + "_ScaledDotProductAttention", {input_1});
  std::string layer_selfattentionencoder =
      g.template AddNode<fetch::ml::layers::SelfAttentionEncoder<TensorType>>(
          name + "_SelfAttentionEncoder", {input_1});
  std::string layer_skipgram =
      g.template AddNode<fetch::ml::layers::SkipGram<TensorType>>(name + "_SkipGram", {input_1});

  // Generate input
  TensorType data1 = TensorType::FromString(R"(-1 , 0 , 1, 2 , 3 , 4)");
  TensorType data2 = TensorType::FromString(R"(-20,-10, 0, 10, 20, 30)");
  g.SetInput(input_1, data1);
  g.SetInput(input_2, data2);

  // serialise the graph
  fetch::ml::GraphSaveableParams<TypeParam>      gsp1 = g.GetGraphSaveableParams();
  fetch::serializers::LargeObjectSerializeHelper b;
  b.Serialize(gsp1);

  // deserialise to a new graph
  auto gsp2 = std::make_shared<fetch::ml::GraphSaveableParams<TypeParam>>();
  b.Deserialize(*gsp2);
  EXPECT_EQ(gsp1.connections, gsp2->connections);

  //  for (auto const &gsp2_node_pair : gsp2->nodes)
  //  {
  //    auto gsp2_node = gsp2_node_pair.second;
  //    auto gsp1_node = gsp1.nodes[gsp2_node_pair.first];
  //
  //    EXPECT_TRUE(gsp1_node->operation_type == gsp2_node->operation_type);
  //  }
  //
  //  auto g2 = std::make_shared<GraphType>();
  //  fetch::ml::utilities::BuildGraph<TensorType>(*gsp2, g2);
  //
  //  TensorType data   = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
  //  TensorType labels = TensorType::FromString("1; 2; 3; 4; 5; 6; 7; 8; 9; 100");
  //
  //  g->SetInput("Input", data.Transpose());
  //  g2->SetInput("Input", data.Transpose());
  //
  //  TensorType prediction  = g->Evaluate(output);
  //  TensorType prediction2 = g2->Evaluate(output);
  //
  //  // test correct values
  //  EXPECT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
  //                                  fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
