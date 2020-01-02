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

#include "core/serializers/main_serializer.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SerializersTestWithInt : public ::testing::Test
{
};

template <typename T>
class SerializersTestNoInt : public ::testing::Test
{
};

TYPED_TEST_CASE(SerializersTestWithInt, math::test::TensorIntAndFloatingTypes);
TYPED_TEST_CASE(SerializersTestNoInt, math::test::TensorFloatingTypes);

TYPED_TEST(SerializersTestWithInt, serialize_empty_state_dict)
{
  fetch::ml::StateDict<TypeParam>       sd1;
  fetch::serializers::MsgPackSerializer b;
  b << sd1;
  b.seek(0);
  fetch::ml::StateDict<TypeParam> sd2;
  b >> sd2;
  EXPECT_EQ(sd1, sd2);
}

TYPED_TEST(SerializersTestNoInt, serialize_state_dict)
{
  // Generate a plausible state dict out of a fully connected layer
  fetch::ml::layers::FullyConnected<TypeParam> fc(10, 10);
  struct fetch::ml::StateDict<TypeParam>       sd1 = fc.StateDict();
  fetch::serializers::MsgPackSerializer        b;
  b << sd1;
  b.seek(0);
  fetch::ml::StateDict<TypeParam> sd2;
  b >> sd2;
  EXPECT_EQ(sd1, sd2);
}

TYPED_TEST(SerializersTestWithInt, serialize_empty_graph_saveable_params)
{
  fetch::ml::GraphSaveableParams<TypeParam> gsp1;
  fetch::serializers::MsgPackSerializer     b;
  b << gsp1;
  b.seek(0);
  fetch::ml::GraphSaveableParams<TypeParam> gsp2;
  b >> gsp2;
  EXPECT_EQ(gsp1.connections, gsp2.connections);
  EXPECT_EQ(gsp1.nodes, gsp2.nodes);
}

TYPED_TEST(SerializersTestNoInt, serialize_graph_saveable_params)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using GraphType  = typename fetch::ml::Graph<TensorType>;

  fetch::ml::RegularisationType regulariser = fetch::ml::RegularisationType::L1;
  auto                          reg_rate    = fetch::math::Type<DataType>("0.01");

  // Prepare graph with fairly random architecture
  auto g = std::make_shared<GraphType>();

  std::string input = g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string label_name =
      g->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("label", {});

  std::string layer_1 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC1", {input}, 10u, 20u, fetch::ml::details::ActivationType::RELU, regulariser, reg_rate);
  std::string layer_2 = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC2", {layer_1}, 20u, 10u, fetch::ml::details::ActivationType::RELU, regulariser, reg_rate);
  std::string output = g->template AddNode<fetch::ml::layers::FullyConnected<TensorType>>(
      "FC3", {layer_2}, 10u, 10u, fetch::ml::details::ActivationType::SOFTMAX, regulariser,
      reg_rate);

  // Add loss function
  std::string error_output = g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TensorType>>(
      "num_error", {output, label_name});

  /// make a prediction and do nothing with it
  TensorType tmp_data = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
  g->SetInput("Input", tmp_data.Transpose());
  TensorType tmp_prediction = g->Evaluate(output);

  fetch::ml::GraphSaveableParams<TypeParam>      gsp1 = g->GetGraphSaveableParams();
  fetch::serializers::LargeObjectSerializeHelper b;
  b.Serialize(gsp1);

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

  TensorType data   = TensorType::FromString("1, 2, 3, 4, 5, 6, 7, 8, 9, 10");
  TensorType labels = TensorType::FromString("1; 2; 3; 4; 5; 6; 7; 8; 9; 100");

  g->SetInput("Input", data.Transpose());
  g2->SetInput("Input", data.Transpose());

  TensorType prediction  = g->Evaluate(output);
  TensorType prediction2 = g2->Evaluate(output);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(prediction2, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  // train g
  g->SetInput(label_name, labels);
  g->Evaluate(error_output);
  g->BackPropagate(error_output);
  auto grads = g->GetGradients();
  for (auto &grad : grads)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  g->ApplyGradients(grads);

  // train g2
  g2->SetInput(label_name, labels);
  g2->Evaluate(error_output);
  g2->BackPropagate(error_output);
  auto grads2 = g2->GetGradients();
  for (auto &grad : grads2)
  {
    grad *= fetch::math::Type<DataType>("-0.1");
  }
  g2->ApplyGradients(grads2);

  g->SetInput("Input", data.Transpose());
  TensorType prediction3 = g->Evaluate(output);

  g2->SetInput("Input", data.Transpose());
  TensorType prediction4 = g2->Evaluate(output);

  EXPECT_FALSE(prediction.AllClose(prediction3, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));

  EXPECT_TRUE(prediction3.AllClose(prediction4, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
