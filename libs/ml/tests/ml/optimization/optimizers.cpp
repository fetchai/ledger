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
#include "ml/ops/multiply.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/subtract.hpp"
#include "ml/optimization/optimizer.hpp"

#include "ml/layers/self_attention.hpp"

#include <gtest/gtest.h>

template <typename T>
class OptimizersTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
        fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(OptimizersTest, MyTypes);

TYPED_TEST(OptimizersTest, diamond_graph_training)  // output=(input1*input2)-(input1^2)
{
using DataType = typename TypeParam::Type;
using SizeType  = typename TypeParam::SizeType;
using ArrayType = TypeParam;

// Generate input
ArrayType data1        = ArrayType::FromString("-1,0,1,2,3,4");
ArrayType data2        = ArrayType::FromString("-20,-10, 0, 10, 20, 30");
ArrayType error_signal = ArrayType::FromString("-1,0,1,2,3,4");
ArrayType grad1        = ArrayType::FromString("1,  0,  1,  4,  9, 16");
ArrayType grad2        = ArrayType::FromString("18, 0, -2, 12, 42, 88");

// Create graph
std::string                 name = "Diamond";
std::shared_ptr<fetch::ml::Graph<TypeParam>> g=std::shared_ptr<fetch::ml::Graph<TypeParam>>(new fetch::ml::Graph<TypeParam>());

std::string input_name1 =
        g->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Input1", {});

std::string input_name2 =
        g->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Input2", {});

std::string op1_name = g->template AddNode<fetch::ml::ops::Multiply<ArrayType>>(
name + "_Op1", {input_name1, input_name1});
std::string op2_name = g->template AddNode<fetch::ml::ops::Multiply<ArrayType>>(
name + "_Op2", {input_name1, input_name2});

std::string output_name =
        g->template AddNode<fetch::ml::ops::Subtract<ArrayType>>(name + "_Op3", {op2_name, op1_name});

// Forward
g->SetInput(input_name1, data1);
g->SetInput(input_name2, data2);

fetch::ml::Optimizer<TypeParam> optimizer(g,output_name,DataType{0.1f});

for(SizeType i{0};i<10;i++)
{
std::cout<<"Weights before: "<<g->GetWeights()[0].ToString()<<std::endl;

//    g->SetInput(input_name1, g->GetWeights()[0].Copy());
//    g->SetInput(input_name2, g->GetWeights()[1].Copy());

ArrayType loss = optimizer.Step();
std::cout<<"Loss: "<<loss.ToString()<<std::endl;
std::cout<<"Weights after: "<<g->GetWeights()[0].ToString()<<std::endl;

}


// Test Weights
std::vector<TypeParam> weights = g->GetWeights();
EXPECT_EQ(weights.size(), 2);
ASSERT_TRUE(
        weights[0].AllClose(data2, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
ASSERT_TRUE(
        weights[1].AllClose(data1, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));


/*
// Change data2
data2                       = ArrayType::FromString("-2, -1, 0, 1, 2, 3");
error_signal                = ArrayType::FromString("-0.1,0,0.1,0.2,0.3,0.4");
ArrayType weights1_expected = ArrayType::FromString("-1,-1,1,5,11,19");
ArrayType weights2_expected = ArrayType::FromString("17, 0, -1, 14, 45, 92");
grad1                       = ArrayType::FromString("-1.7,0,-0.1,2.8,13.5,36.8");
grad2                       = ArrayType::FromString("3.5, 0, 0.3, -4.6, -23.7, -66");

g.SetInput(input_name2, data2);

// Apply gradient;
g.ApplyGradients(gradients);

// Recompute graph
output = g.Evaluate("Diamond_Op3");

// Calculate Gradient
g.BackPropagate(output_name, error_signal);

// Test Weights
std::vector<TypeParam> weights2 = g.GetWeights();
EXPECT_EQ(weights2.size(), 2);
ASSERT_TRUE(weights2[0].AllClose(weights1_expected, static_cast<DataType>(1e-5f),
static_cast<DataType>(1e-5f)));
ASSERT_TRUE(weights2[1].AllClose(weights2_expected, static_cast<DataType>(1e-5f),
static_cast<DataType>(1e-5f)));

// Test gradient
std::vector<TypeParam> gradients2 = g.GetGradients();
EXPECT_EQ(gradients2.size(), 2);
ASSERT_TRUE(
        gradients2[0].AllClose(grad1, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
ASSERT_TRUE(
        gradients2[1].AllClose(grad2, static_cast<DataType>(1e-5f), static_cast<DataType>(1e-5f)));
        */
}