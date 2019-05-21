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

#include "ml/graph.hpp"
#include "math/tensor.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/placeholder.hpp"

#include "ml/layers/self_attention.hpp"

#include <gtest/gtest.h>

template <typename T>
class GraphTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(GraphTest, MyTypes);

TYPED_TEST(GraphTest, node_placeholder)
{
  using ArrayType = TypeParam;

  fetch::ml::Graph<ArrayType> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});

  ArrayType data = ArrayType::FromString("1, 2, 3, 4, 5, 6, 7, 8");
  ArrayType gt   = ArrayType::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  g.SetInput("Input", data);
  ArrayType prediction = g.Evaluate("Input");

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(GraphTest, node_relu)
{
  using ArrayType = TypeParam;

  fetch::ml::Graph<ArrayType> g;
  g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  g.template AddNode<fetch::ml::ops::Relu<ArrayType>>("Relu", {"Input"});

  ArrayType data =
      ArrayType::FromString("0, -1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16");
  ArrayType gt = ArrayType::FromString("0, 0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12, 0, 14, 0, 16");

  g.SetInput("Input", data);
  ArrayType prediction = g.Evaluate("Relu");

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(GraphTest, getStateDict)
{
  using ArrayType = TypeParam;

  fetch::ml::Graph<ArrayType>     g;
  fetch::ml::StateDict<ArrayType> sd = g.StateDict();

  EXPECT_EQ(sd.weights_, nullptr);
  EXPECT_TRUE(sd.dict_.empty());
}

TYPED_TEST(GraphTest, no_such_node_test)  // Use the class as a Node
{
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  fetch::ml::Graph<ArrayType> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>("Input", {});
  g.template AddNode<fetch::ml::layers::SelfAttention<ArrayType>>("SelfAttention", {"Input"}, 50u,
                                                                  42u, 10u);

  ArrayType data(std::vector<SizeType>({5, 10}));
  g.SetInput("Input", data);

  ASSERT_ANY_THROW(g.Evaluate("FullyConnected"));
}

TYPED_TEST(GraphTest, diamond_shaped_graph)  // Evaluate graph output=(input+input)+(input+input)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  // Generate input
  ArrayType data = ArrayType::FromString("-1,0,1,2,3,4");
  ArrayType gt   = ArrayType::FromString("-4,0,4,8,12,16");

  // Create graph
  std::string                 name = "Diamond";
  fetch::ml::Graph<TypeParam> g;

  std::string input_name =
      g.template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});

  std::string add1_name =
      g.template AddNode<fetch::ml::ops::Add<ArrayType>>(name + "_Add1", {input_name, input_name});
  std::string add2_name =
      g.template AddNode<fetch::ml::ops::Add<ArrayType>>(name + "_Add2", {input_name, input_name});

  std::string output_name =
      g.template AddNode<fetch::ml::ops::Add<ArrayType>>(name + "_Add3", {add1_name, add2_name});

  // Evaluate
  g.SetInput(input_name, data);
  TypeParam output = g.Evaluate("Diamond_Add3");

  // Test correct values
  ASSERT_EQ(output.shape(), data.shape());
  ASSERT_TRUE(output.AllClose(gt, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
}