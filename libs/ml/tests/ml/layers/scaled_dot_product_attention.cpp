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
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

template <typename T>
class ScaledDotProductAttention : public ::testing::Test
{
};
using MyTypes  = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>/*,
                                    fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                    fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>*/>;
TYPED_TEST_CASE(ScaledDotProductAttention, MyTypes);

TYPED_TEST(ScaledDotProductAttention, input_output_dimension_check)  // Use the class as a subgraph
{
	using DataType = typename TypeParam::Type;
	
	fetch::ml::Graph<TypeParam> g;
	
	std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
	std::string key   = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
	std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
	g.template AddNode<fetch::ml::layers::ScaledDotProductAttention<TypeParam>>("ScaledDotProductAttention", {query, key, value}, 4u, DataType(0.1));
	TypeParam query_data = TypeParam::FromString("1, 2; 2, 1;2, 4");
	query_data.Reshape({2, 2, 1});
	TypeParam key_data   = TypeParam::FromString("0.5, 3; 1, 7; 3, 6");
	key_data.Reshape({2, 2, 1});
	std::cout << "key_data.View(0).Copy().ToString(): " << key_data.View(0).Copy().ToString() << std::endl;
	TypeParam value_data = key_data;
	g.SetInput(query, query_data);
	g.SetInput(key, key_data);
	g.SetInput(value, value_data);
	
	TypeParam prediction = g.Evaluate("ScaledDotProductAttention", false);
	ASSERT_EQ(prediction.shape()[0], 3);
	ASSERT_EQ(prediction.shape()[1], 7);
	ASSERT_EQ(prediction.shape()[2], 2);
}

TYPED_TEST(ScaledDotProductAttention, self_attention_output_value_check)  // Use the class as a subgraph
{
	using DataType = typename TypeParam::Type;
	
	using DataType = typename TypeParam::Type;
	fetch::ml::Graph<TypeParam> g;
	
	std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
	std::string key   = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
	std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
	g.template AddNode<fetch::ml::layers::ScaledDotProductAttention<TypeParam>>("ScaledDotProductAttention", {query, key, value}, 4u, DataType(0.1));
	TypeParam query_data = TypeParam({4, 7, 2});
	TypeParam key_data   = TypeParam({4, 5, 2});
	TypeParam value_data = TypeParam({3, 5, 2});
	g.SetInput(query, query_data);
	g.SetInput(key, key_data);
	g.SetInput(value, value_data);
	
	TypeParam prediction = g.Evaluate("ScaledDotProductAttention", false);
	ASSERT_EQ(prediction.shape()[0], 3);
	ASSERT_EQ(prediction.shape()[1], 7);
	ASSERT_EQ(prediction.shape()[2], 2);
}