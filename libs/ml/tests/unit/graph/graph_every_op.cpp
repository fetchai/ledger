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

#include "ml/core/graph.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
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

// metrics
#include "ml/ops/metrics/categorical_accuracy.hpp"

// layers
#include "gtest/gtest.h"
#include "ml/layers/PRelu.hpp"
#include "ml/layers/convolution_1d.hpp"
#include "ml/layers/convolution_2d.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/multihead_attention.hpp"
#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/layers/self_attention_encoder.hpp"
#include "ml/layers/skip_gram.hpp"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class GraphRebuildTest : public ::testing::Test
{
};

TYPED_TEST_CASE(GraphRebuildTest, math::test::HighPrecisionTensorFloatingTypes);

template <typename OpType, typename GraphPtrType, typename... Params>
std::string AddOp(GraphPtrType g, std::vector<std::string> input_nodes, Params... params)
{
  return g->template AddNode<OpType>("", input_nodes, params...);
}

template <typename GraphPtrType, typename TensorType>
void ComparePrediction(GraphPtrType g, GraphPtrType g2, std::string node_name)
{
  using DataType         = typename TensorType::Type;
  TensorType prediction  = g->Evaluate(node_name);
  TensorType prediction2 = g2->Evaluate(node_name);
  EXPECT_TRUE(prediction.AllClose(prediction2, DataType{0}, DataType{0}));
}

TYPED_TEST(GraphRebuildTest, graph_rebuild_every_op)
{
  using TensorType   = TypeParam;
  using DataType     = typename TensorType::Type;
  using GraphType    = fetch::ml::Graph<TypeParam>;
  using GraphPtrType = std::shared_ptr<GraphType>;

  // setup input data
  TensorType data1       = TensorType::FromString(R"(1 , 1 , 1, 2 , 3 , 4)");
  TensorType data2       = TensorType::FromString(R"(-20,-10, 1, 10, 20, 30)");
  TensorType data_3d     = TensorType::FromString(R"(1, 1, 1, 2 , 3 , 2, 1, 2)");
  TensorType data_4d     = TensorType::FromString(R"(-1, 1, 1, 2 , 3 , 2, 1, 2)");
  TensorType data_5d     = TensorType::FromString(R"(-1, 1, 1, 2 , 3 , 2, 1, 2)");
  TensorType data_binary = TensorType::FromString(R"(1 , 1 , 0, 0 , 0 , 1)");
  TensorType data_logits = TensorType::FromString(R"(0.2 , 0.2 , 0.2, 0.2 , 0.1 , 0.1)");
  TensorType data_embed({5, 5});
  TensorType query_data = TensorType({12, 25, 4});
  query_data.Fill(DataType{0});
  TensorType key_data   = query_data;
  TensorType value_data = query_data;
  TensorType mask_data  = TensorType({25, 25, 4});
  data_3d.Reshape({2, 2, 2});
  data_4d.Reshape({2, 2, 2, 1});
  data_5d.Reshape({2, 2, 2, 1, 1});
  TensorType data_1_2_4 = data1.Copy();
  data_1_2_4.Reshape({2, 4});

  // Create graph
  std::string name = "Graph";
  auto        g    = std::make_shared<GraphType>();

  // placeholder inputs
  std::string input_1                = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_1_transpose      = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_1_2_4            = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_2                = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_3d               = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_4d               = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_5d               = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_binary           = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_binary_transpose = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_logits           = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_logits_transpose = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_query            = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_key              = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_value            = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string input_mask             = AddOp<ops::PlaceHolder<TensorType>>(g, {});

  // ordinary ops
  std::string abs          = AddOp<ops::Abs<TensorType>>(g, {input_1});
  std::string add          = AddOp<ops::Add<TensorType>>(g, {input_1, input_2});
  std::string avg1         = AddOp<ops::AvgPool1D<TensorType>>(g, {input_3d}, 1, 1);
  std::string avg2         = AddOp<ops::AvgPool2D<TensorType>>(g, {input_4d}, 1, 1);
  std::string concat       = AddOp<ops::Concatenate<TensorType>>(g, {input_1, input_2}, 0);
  std::string constant     = AddOp<ops::Constant<TensorType>>(g, {});
  std::string conv1d       = AddOp<ops::Convolution1D<TensorType>>(g, {input_3d, input_4d});
  std::string conv2d       = AddOp<ops::Convolution2D<TensorType>>(g, {input_4d, input_5d});
  std::string divide       = AddOp<ops::Divide<TensorType>>(g, {input_1, input_2});
  std::string embed        = AddOp<ops::Embeddings<TensorType>>(g, {input_1}, data_embed);
  std::string exp          = AddOp<ops::Exp<TensorType>>(g, {input_1});
  std::string flatten      = AddOp<ops::Flatten<TensorType>>(g, {input_1});
  std::string layernorm_op = AddOp<ops::LayerNorm<TensorType>>(g, {input_1});
  std::string log          = AddOp<ops::Log<TensorType>>(g, {input_1});
  std::string maskfill     = AddOp<ops::MaskFill<TensorType>>(g, {input_1, input_1}, DataType{0});
  std::string matmul      = AddOp<ops::MatrixMultiply<TensorType>>(g, {input_1, input_1_transpose});
  std::string maxpool     = AddOp<ops::MaxPool<TensorType>>(g, {input_3d}, 1, 1);
  std::string maxpool1d   = AddOp<ops::MaxPool1D<TensorType>>(g, {input_3d}, 1, 1);
  std::string maxpool2d   = AddOp<ops::MaxPool2D<TensorType>>(g, {input_4d}, 1, 1);
  std::string maximum     = AddOp<ops::Maximum<TensorType>>(g, {input_1, input_2});
  std::string multiply    = AddOp<ops::Multiply<TensorType>>(g, {input_1, input_2});
  std::string onehot      = AddOp<ops::OneHot<TensorType>>(g, {input_1}, data1.size());
  std::string placeholder = AddOp<ops::PlaceHolder<TensorType>>(g, {});
  std::string prelu       = AddOp<ops::PReluOp<TensorType>>(g, {input_1, input_1_transpose});
  std::string reducemean  = AddOp<ops::ReduceMean<TensorType>>(g, {input_1}, 0);
  std::string slice       = AddOp<ops::Slice<TensorType>>(g, {input_1}, 0, 0);
  std::string sqrt        = AddOp<ops::Sqrt<TensorType>>(g, {input_1});
  std::string squeeze     = AddOp<ops::Squeeze<TensorType>>(g, {input_1});
  std::string switchop    = AddOp<ops::Switch<TensorType>>(g, {input_1, input_1, input_1});
  std::string tanh        = AddOp<ops::TanH<TensorType>>(g, {input_1});
  std::string transpose   = AddOp<ops::Transpose<TensorType>>(g, {input_1});
  std::string topk        = AddOp<ops::TopK<TensorType>>(g, {input_1_2_4}, 2);
  std::string weights     = AddOp<ops::Weights<TensorType>>(g, {});

  // activations
  std::string dropout =
      AddOp<ops::Dropout<TensorType>>(g, {input_1}, fetch::math::Type<DataType>("0.9"));
  std::string elu  = AddOp<ops::Elu<TensorType>>(g, {input_1}, fetch::math::Type<DataType>("0.9"));
  std::string gelu = AddOp<ops::Gelu<TensorType>>(g, {input_1});
  std::string leakyrelu  = AddOp<ops::LeakyRelu<TensorType>>(g, {input_1});
  std::string logsigmoid = AddOp<ops::LogSigmoid<TensorType>>(g, {input_1});
  std::string logsoftmax = AddOp<ops::LogSoftmax<TensorType>>(g, {input_1});
  std::string randomisedrelu =
      AddOp<ops::RandomisedRelu<TensorType>>(g, {input_1}, DataType{0}, DataType{1});
  std::string relu    = AddOp<ops::Relu<TensorType>>(g, {input_1});
  std::string sigmoid = AddOp<ops::Sigmoid<TensorType>>(g, {input_1});
  std::string softmax = AddOp<ops::Softmax<TensorType>>(g, {input_1});

  // Loss functions
  std::string cel  = AddOp<ops::CrossEntropyLoss<TensorType>>(g, {input_logits, input_binary});
  std::string mse  = AddOp<ops::MeanSquareErrorLoss<TensorType>>(g, {input_1, input_2});
  std::string scel = AddOp<ops::SoftmaxCrossEntropyLoss<TensorType>>(
      g, {input_logits_transpose, input_binary_transpose});

  // Metrics
  std::string acc = AddOp<ops::CategoricalAccuracy<TensorType>>(
      g, {input_logits_transpose, input_binary_transpose});

  // Layers
  std::string layer_layernorm =
      AddOp<layers::LayerNorm<TensorType>>(g, {input_1}, std::vector<math::SizeType>({1}));
  std::string layer_conv1d = AddOp<layers::Convolution1D<TensorType>>(g, {input_3d}, 1, 2, 1, 1);
  std::string layer_conv2d = AddOp<layers::Convolution2D<TensorType>>(g, {input_4d}, 1, 2, 1, 1);
  std::string layer_fc1    = AddOp<layers::FullyConnected<TensorType>>(g, {input_1}, 1, 1);
  std::string layer_mh     = AddOp<layers::MultiheadAttention<TensorType>>(
      g, {input_query, input_key, input_value, input_mask}, 4, 12);
  std::string layer_prelu = AddOp<layers::PRelu<TensorType>>(g, {input_1}, 1);
  std::string layer_scaleddotproductattention =
      AddOp<layers::ScaledDotProductAttention<TensorType>>(
          g, {input_query, input_key, input_value, input_mask}, 4);
  std::string layer_selfattentionencoder =
      AddOp<layers::SelfAttentionEncoder<TensorType>>(g, {input_query, input_mask}, 4, 12, 24);
  std::string layer_skipgram =
      AddOp<layers::SkipGram<TensorType>>(g, {input_1, input_1}, 1, 1, 10, 10);

  // assign input data
  g->SetInput(input_1, data1);
  g->SetInput(input_1_transpose, data1.Copy().Transpose());
  g->SetInput(input_1_2_4, data_1_2_4);
  g->SetInput(input_2, data2);
  g->SetInput(input_3d, data_3d);
  g->SetInput(input_4d, data_4d);
  g->SetInput(input_5d, data_5d);
  g->SetInput(constant, data1);
  g->SetInput(placeholder, data1);
  g->SetInput(weights, data1);
  g->SetInput(input_binary, data_binary);
  g->SetInput(input_binary_transpose, data_binary.Copy().Transpose());
  g->SetInput(input_logits, data_logits);
  g->SetInput(input_logits_transpose, data_logits.Copy().Transpose());
  g->SetInput(input_query, query_data);
  g->SetInput(input_key, key_data);
  g->SetInput(input_value, value_data);
  g->SetInput(input_mask, mask_data);
  g->Compile();

  // serialise the graph
  fetch::ml::GraphSaveableParams<TypeParam>      gsp1 = g->GetGraphSaveableParams();
  fetch::serializers::LargeObjectSerializeHelper b;
  b.Serialize(gsp1);

  // deserialise to a new graph
  auto gsp2 = std::make_shared<fetch::ml::GraphSaveableParams<TypeParam>>();
  b.Deserialize(*gsp2);
  EXPECT_EQ(gsp1.connections, gsp2->connections);

  for (auto const &gsp2_node_pair : gsp2->nodes)
  {
    auto gsp2_node = gsp2_node_pair.second;
    auto gsp1_node = gsp1.nodes[gsp2_node_pair.first];

    EXPECT_TRUE(gsp1_node->operation_type == gsp2_node->operation_type);
  }

  auto g2 = std::make_shared<GraphType>();
  fetch::ml::utilities::BuildGraph<TensorType>(*gsp2, g2);

  // evaluate both graphs to ensure outputs are identical
  g2->SetInput(input_1, data1);
  g2->SetInput(input_1_transpose, data1.Copy().Transpose());
  g2->SetInput(input_1_2_4, data_1_2_4);
  g2->SetInput(input_2, data2);
  g2->SetInput(input_3d, data_3d);
  g2->SetInput(input_4d, data_4d);
  g2->SetInput(input_5d, data_5d);
  g2->SetInput(constant, data1);
  g2->SetInput(placeholder, data1);
  g2->SetInput(weights, data1);
  g2->SetInput(input_binary, data_binary);
  g2->SetInput(input_binary_transpose, data_binary.Copy().Transpose());
  g2->SetInput(input_logits, data_logits);
  g2->SetInput(input_logits_transpose, data_logits.Copy().Transpose());
  g2->SetInput(input_query, query_data);
  g2->SetInput(input_key, key_data);
  g2->SetInput(input_value, value_data);
  g2->SetInput(input_mask, mask_data);
  g2->Compile();

  // weak tests that all ops produce the same value on both graphs
  // more thorough tests should be implemented in each test op file

  // ordinary ops
  ComparePrediction<GraphPtrType, TensorType>(g, g2, input_1);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, input_2);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, abs);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, add);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, avg1);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, avg2);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, concat);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, constant);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, conv1d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, conv2d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, divide);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, embed);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, exp);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, flatten);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layernorm_op);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, log);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maskfill);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, matmul);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maxpool);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maxpool1d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maxpool2d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, maximum);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, multiply);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, onehot);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, placeholder);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, prelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, reducemean);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, slice);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, sqrt);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, squeeze);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, switchop);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, tanh);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, transpose);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, topk);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, weights);

  // activations
  ComparePrediction<GraphPtrType, TensorType>(g, g2, dropout);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, elu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, gelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, leakyrelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, logsigmoid);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, logsoftmax);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, randomisedrelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, relu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, sigmoid);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, softmax);

  // Loss functions
  ComparePrediction<GraphPtrType, TensorType>(g, g2, cel);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, mse);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, scel);

  // Metrics
  ComparePrediction<GraphPtrType, TensorType>(g, g2, acc);

  // Layers
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_layernorm);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_conv1d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_conv2d);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_fc1);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_mh);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_prelu);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_scaleddotproductattention);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_selfattentionencoder);
  ComparePrediction<GraphPtrType, TensorType>(g, g2, layer_skipgram);
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
