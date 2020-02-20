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

#include "serialiser_includes.hpp"
#include "serialiser_test_utils.hpp"

namespace {

template <typename T>
class SerializersTestWithInt : public ::testing::Test
{
};

template <typename T>
class SerializersTestNoInt : public ::testing::Test
{
};

template <typename T>
class GraphRebuildTest : public ::testing::Test
{
};

// TYPED_TEST_SUITE(SaveParamsTest, ::fetch::math::test::TensorFloatingTypes,);
TYPED_TEST_SUITE(SerializersTestWithInt, ::fetch::math::test::TensorIntAndFloatingTypes, );
TYPED_TEST_SUITE(SerializersTestNoInt, ::fetch::math::test::TensorFloatingTypes, );
TYPED_TEST_SUITE(GraphRebuildTest, ::fetch::math::test::HighPrecisionTensorFloatingTypes, );

//////////////////////////////
/// GRAPH OP SERIALISATION ///
//////////////////////////////

template <typename OpKind, typename GraphPtrType, typename... Params>
std::string AddOp(GraphPtrType g, std::vector<std::string> input_nodes, Params... params)
{
  return g->template AddNode<OpKind>("", input_nodes, params...);
}

template <typename GraphPtrType, typename TensorType>
void ComparePrediction(GraphPtrType g, GraphPtrType g2, std::string node_name)
{
  using DataType         = typename TensorType::Type;
  TensorType prediction  = g->Evaluate(node_name);
  TensorType prediction2 = g2->Evaluate(node_name);
  EXPECT_TRUE(prediction.AllClose(prediction2, DataType{0}, DataType{0}));
}

TYPED_TEST(SerializersTestWithInt, serialize_empty_graph_saveable_params)
{
  ::fetch::ml::GraphSaveableParams<TypeParam> gsp1;
  ::fetch::serializers::MsgPackSerializer     b;
  b << gsp1;
  b.seek(0);
  ::fetch::ml::GraphSaveableParams<TypeParam> gsp2;
  b >> gsp2;
  EXPECT_EQ(gsp1.connections, gsp2.connections);
  EXPECT_EQ(gsp1.nodes, gsp2.nodes);
}

TYPED_TEST(SerializersTestNoInt, serialize_graph_saveable_params)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using GraphType  = typename ::fetch::ml::Graph<TensorType>;

  ::fetch::ml::RegularisationType regulariser = ::fetch::ml::RegularisationType::L1;
  auto                            reg_rate    = ::fetch::math::Type<DataType>("0.01");

  // Prepare graph with fairly random architecture
  auto g = std::make_shared<GraphType>();

  std::string input = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string label_name =
      g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("label", {});

  std::string layer_1 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {input}, 10u, 20u, ::fetch::ml::details::ActivationType::RELU, regulariser, reg_rate);
  std::string layer_2 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC2", {layer_1}, 20u, 10u, ::fetch::ml::details::ActivationType::RELU, regulariser,
      reg_rate);
  std::string output = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC3", {layer_2}, 10u, 10u, ::fetch::ml::details::ActivationType::SOFTMAX, regulariser,
      reg_rate);

  // Add loss function
  std::string error_output = g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
      "num_error", {output, label_name});

  /// make a prediction and do nothing with it
  TensorType tmp_data = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
  g->SetInput("Input", tmp_data.Transpose());
  g->Compile();

  TensorType tmp_prediction = g->Evaluate(output);

  ::fetch::ml::GraphSaveableParams<TypeParam>      gsp1 = g->GetGraphSaveableParams();
  ::fetch::serializers::LargeObjectSerializeHelper b;
  b.Serialize(gsp1);

  auto gsp2 = std::make_shared<::fetch::ml::GraphSaveableParams<TypeParam>>();

  b.Deserialize(*gsp2);
  EXPECT_EQ(gsp1.connections, gsp2->connections);

  for (auto const &gsp2_node_pair : gsp2->nodes)
  {
    auto gsp2_node = gsp2_node_pair.second;
    auto gsp1_node = gsp1.nodes[gsp2_node_pair.first];

    EXPECT_TRUE(gsp1_node->operation_type == gsp2_node->operation_type);
  }

  auto g2 = std::make_shared<GraphType>();
  ::fetch::ml::utilities::BuildGraph<TensorType>(*gsp2, g2);

  TensorType data   = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
  TensorType labels = TensorType::FromString("1; 2; 3; 4; 5; 6; 7; 8; 9; 100");

  g->SetInput("Input", data.Transpose());
  g2->SetInput("Input", data.Transpose());

  TensorType prediction  = g->Evaluate(output);
  TensorType prediction2 = g2->Evaluate(output);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(prediction2, ::fetch::math::function_tolerance<DataType>(),
                                  ::fetch::math::function_tolerance<DataType>()));

  // train g
  g->SetInput(label_name, labels);
  g->Evaluate(error_output);
  g->BackPropagate(error_output);
  auto grads = g->GetGradients();
  for (auto &grad : grads)
  {
    grad *= ::fetch::math::Type<DataType>("-0.1");
  }
  g->ApplyGradients(grads);

  // train g2
  g2->SetInput(label_name, labels);
  g2->Evaluate(error_output);
  g2->BackPropagate(error_output);
  auto grads2 = g2->GetGradients();
  for (auto &grad : grads2)
  {
    grad *= ::fetch::math::Type<DataType>("-0.1");
  }
  g2->ApplyGradients(grads2);

  g->SetInput("Input", data.Transpose());
  TensorType prediction3 = g->Evaluate(output);

  g2->SetInput("Input", data.Transpose());
  TensorType prediction4 = g2->Evaluate(output);

  EXPECT_FALSE(prediction.AllClose(prediction3, ::fetch::math::function_tolerance<DataType>(),
                                   ::fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(prediction3.AllClose(prediction4, ::fetch::math::function_tolerance<DataType>(),
                                   ::fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(GraphRebuildTest, graph_rebuild_every_op)
{
  using TensorType   = TypeParam;
  using DataType     = typename TensorType::Type;
  using GraphType    = ::fetch::ml::Graph<TypeParam>;
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
  std::string input_1                = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_1_transpose      = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_1_2_4            = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_2                = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_3d               = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_4d               = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_5d               = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_binary           = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_binary_transpose = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_logits           = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_logits_transpose = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_query            = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_key              = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_value            = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string input_mask             = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});

  // ordinary ops
  std::string abs      = AddOp<fetch::ml::ops::Abs<TensorType>>(g, {input_1});
  std::string add      = AddOp<fetch::ml::ops::Add<TensorType>>(g, {input_1, input_2});
  std::string avg1     = AddOp<fetch::ml::ops::AvgPool1D<TensorType>>(g, {input_3d}, 1, 1);
  std::string avg2     = AddOp<fetch::ml::ops::AvgPool2D<TensorType>>(g, {input_4d}, 1, 1);
  std::string concat   = AddOp<fetch::ml::ops::Concatenate<TensorType>>(g, {input_1, input_2}, 0);
  std::string constant = AddOp<fetch::ml::ops::Constant<TensorType>>(g, {});
  std::string conv1d   = AddOp<fetch::ml::ops::Convolution1D<TensorType>>(g, {input_3d, input_4d});
  std::string conv2d   = AddOp<fetch::ml::ops::Convolution2D<TensorType>>(g, {input_4d, input_5d});
  std::string divide   = AddOp<fetch::ml::ops::Divide<TensorType>>(g, {input_1, input_2});
  std::string embed    = AddOp<fetch::ml::ops::Embeddings<TensorType>>(g, {input_1}, data_embed);
  std::string exp      = AddOp<fetch::ml::ops::Exp<TensorType>>(g, {input_1});
  std::string flatten  = AddOp<fetch::ml::ops::Flatten<TensorType>>(g, {input_1});
  std::string layernorm_op = AddOp<fetch::ml::ops::LayerNorm<TensorType>>(g, {input_1});
  std::string log          = AddOp<fetch::ml::ops::Log<TensorType>>(g, {input_1});
  std::string maskfill =
      AddOp<fetch::ml::ops::MaskFill<TensorType>>(g, {input_1, input_1}, DataType{0});
  std::string matmul =
      AddOp<fetch::ml::ops::MatrixMultiply<TensorType>>(g, {input_1, input_1_transpose});
  std::string maxpool1d   = AddOp<fetch::ml::ops::MaxPool1D<TensorType>>(g, {input_3d}, 1, 1);
  std::string maxpool2d   = AddOp<fetch::ml::ops::MaxPool2D<TensorType>>(g, {input_4d}, 1, 1);
  std::string maximum     = AddOp<fetch::ml::ops::Maximum<TensorType>>(g, {input_1, input_2});
  std::string multiply    = AddOp<fetch::ml::ops::Multiply<TensorType>>(g, {input_1, input_2});
  std::string onehot      = AddOp<fetch::ml::ops::OneHot<TensorType>>(g, {input_1}, data1.size());
  std::string placeholder = AddOp<fetch::ml::ops::PlaceHolder<TensorType>>(g, {});
  std::string prelu = AddOp<fetch::ml::ops::PReluOp<TensorType>>(g, {input_1, input_1_transpose});
  std::string reducemean = AddOp<fetch::ml::ops::ReduceMean<TensorType>>(g, {input_1}, 0);
  std::string slice      = AddOp<fetch::ml::ops::Slice<TensorType>>(g, {input_1}, 0, 0);
  std::string sqrt       = AddOp<fetch::ml::ops::Sqrt<TensorType>>(g, {input_1});
  std::string squeeze    = AddOp<fetch::ml::ops::Squeeze<TensorType>>(g, {input_1});
  std::string switchop  = AddOp<fetch::ml::ops::Switch<TensorType>>(g, {input_1, input_1, input_1});
  std::string tanh      = AddOp<fetch::ml::ops::TanH<TensorType>>(g, {input_1});
  std::string transpose = AddOp<fetch::ml::ops::Transpose<TensorType>>(g, {input_1});
  std::string topk      = AddOp<fetch::ml::ops::TopK<TensorType>>(g, {input_1_2_4}, 2);
  std::string weights   = AddOp<fetch::ml::ops::Weights<TensorType>>(g, {});

  // activations
  std::string dropout = AddOp<fetch::ml::ops::Dropout<TensorType>>(
      g, {input_1}, ::fetch::math::Type<DataType>("0.9"));
  std::string elu =
      AddOp<fetch::ml::ops::Elu<TensorType>>(g, {input_1}, ::fetch::math::Type<DataType>("0.9"));
  std::string gelu       = AddOp<fetch::ml::ops::Gelu<TensorType>>(g, {input_1});
  std::string leakyrelu  = AddOp<fetch::ml::ops::LeakyRelu<TensorType>>(g, {input_1});
  std::string logsigmoid = AddOp<fetch::ml::ops::LogSigmoid<TensorType>>(g, {input_1});
  std::string logsoftmax = AddOp<fetch::ml::ops::LogSoftmax<TensorType>>(g, {input_1});
  std::string randomisedrelu =
      AddOp<fetch::ml::ops::RandomisedRelu<TensorType>>(g, {input_1}, DataType{0}, DataType{1});
  std::string relu    = AddOp<fetch::ml::ops::Relu<TensorType>>(g, {input_1});
  std::string sigmoid = AddOp<fetch::ml::ops::Sigmoid<TensorType>>(g, {input_1});
  std::string softmax = AddOp<fetch::ml::ops::Softmax<TensorType>>(g, {input_1});

  // Loss functions
  std::string cel =
      AddOp<fetch::ml::ops::CrossEntropyLoss<TensorType>>(g, {input_logits, input_binary});
  std::string mse  = AddOp<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(g, {input_1, input_2});
  std::string scel = AddOp<fetch::ml::ops::SoftmaxCrossEntropyLoss<TensorType>>(
      g, {input_logits_transpose, input_binary_transpose});

  // Metrics
  std::string acc = AddOp<fetch::ml::ops::CategoricalAccuracy<TensorType>>(
      g, {input_logits_transpose, input_binary_transpose});

  // Layers
  std::string layer_layernorm = AddOp<fetch::ml::layers::LayerNorm<TensorType>>(
      g, {input_1}, std::vector<::fetch::math::SizeType>({1}));
  std::string layer_conv1d =
      AddOp<fetch::ml::layers::Convolution1D<TensorType>>(g, {input_3d}, 1, 2, 1, 1);
  std::string layer_conv2d =
      AddOp<fetch::ml::layers::Convolution2D<TensorType>>(g, {input_4d}, 1, 2, 1, 1);
  std::string layer_fc1 = AddOp<fetch::ml::layers::FullyConnected<TensorType>>(g, {input_1}, 1, 1);
  std::string layer_mh  = AddOp<fetch::ml::layers::MultiheadAttention<TensorType>>(
      g, {input_query, input_key, input_value, input_mask}, 4, 12);
  std::string layer_prelu = AddOp<fetch::ml::layers::PRelu<TensorType>>(g, {input_1}, 1);
  std::string layer_scaleddotproductattention =
      AddOp<fetch::ml::layers::ScaledDotProductAttention<TensorType>>(
          g, {input_query, input_key, input_value, input_mask}, 4);
  std::string layer_selfattentionencoder =
      AddOp<fetch::ml::layers::SelfAttentionEncoder<TensorType>>(g, {input_query, input_mask}, 4,
                                                                 12, 24);
  std::string layer_skipgram =
      AddOp<fetch::ml::layers::SkipGram<TensorType>>(g, {input_1, input_1}, 1, 1, 10, 10);

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
  ::fetch::ml::GraphSaveableParams<TypeParam>      gsp1 = g->GetGraphSaveableParams();
  ::fetch::serializers::LargeObjectSerializeHelper b;
  b.Serialize(gsp1);

  // deserialise to a new graph
  auto gsp2 = std::make_shared<::fetch::ml::GraphSaveableParams<TypeParam>>();
  b.Deserialize(*gsp2);
  EXPECT_EQ(gsp1.connections, gsp2->connections);

  for (auto const &gsp2_node_pair : gsp2->nodes)
  {
    auto gsp2_node = gsp2_node_pair.second;
    auto gsp1_node = gsp1.nodes[gsp2_node_pair.first];

    EXPECT_TRUE(gsp1_node->operation_type == gsp2_node->operation_type);
  }

  auto g2 = std::make_shared<GraphType>();
  ::fetch::ml::utilities::BuildGraph<TensorType>(*gsp2, g2);

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

}  // namespace
