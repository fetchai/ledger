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

#include <ml/layers/multihead_attention.hpp>
#include "math/tensor.hpp"
#include "ml/layers/scaled_dot_product_attention.hpp"
#include "ml/ops/loss_functions.hpp"
#include "ml/optimisation/sgd_optimiser.hpp"
#include "ml/regularisers/regulariser.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <chrono>

template <typename T>
class MultiheadAttention : public ::testing::Test
{
};
using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>>;
TYPED_TEST_CASE(MultiheadAttention, MyTypes);

TYPED_TEST(MultiheadAttention, input_output_dimension_check)  // Use the class as a subgraph
{
	using DataType = typename TypeParam::Type;
	using SizeType = typename TypeParam::SizeType;
	
	fetch::ml::Graph<TypeParam> g;
	
	std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
	std::string key = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
	std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
	g.template AddNode<fetch::ml::layers::MultiheadAttention<TypeParam>>(
	 "ScaledDotProductAttention", {query, key, value}, static_cast<SizeType>(8), static_cast<SizeType>(1024),
	 DataType(0.1));
	TypeParam query_data = TypeParam({1024, 512, 64});
	TypeParam key_data = TypeParam({1024, 512, 64});
	TypeParam value_data = TypeParam({1024, 512, 64});
	g.SetInput(query, query_data);
	g.SetInput(key, key_data);
	g.SetInput(value, value_data);
	
	
	auto start = std::chrono::steady_clock::now();
	TypeParam prediction = g.Evaluate("ScaledDotProductAttention", false);
	auto end = std::chrono::steady_clock::now();
	
	std::cout << "Elapsed time in milliseconds : "
	     << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
	     << " ms" << std::endl;
	ASSERT_EQ(prediction.shape().size(), 3);
	ASSERT_EQ(prediction.shape()[0], 1024);
	ASSERT_EQ(prediction.shape()[1], 512);
	ASSERT_EQ(prediction.shape()[2], 64);
}

TYPED_TEST(MultiheadAttention, tmp)  // Use the class as a subgraph
{
	TypeParam one = TypeParam({27, 27, 12});
	TypeParam two = TypeParam({1, 27, 12});
	
	auto three = fetch::math::Softmax(one, 0);
	
}

//
//TYPED_TEST(ScaledDotProductAttention,
//           self_attention_output_value_check)  // Use the class as a subgraph
//{
//  using DataType = typename TypeParam::Type;
//  using SizeType = typename TypeParam::SizeType;
//
//  fetch::ml::Graph<TypeParam> g;
//
//  std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
//  std::string key   = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
//  std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
//  g.template AddNode<fetch::ml::layers::ScaledDotProductAttention<TypeParam>>(
//      "ScaledDotProductAttention", {query, key, value}, static_cast<SizeType>(3), DataType(0.1));
//  TypeParam query_data = TypeParam::FromString("1, 2, 0.5, 0.1; 2, 1, 0.3, -0.2;2, 4, 0, 1");
//  query_data.Reshape({3, 2, 2});
//
//  g.SetInput(query, query_data);
//  g.SetInput(key, query_data);
//  g.SetInput(value, query_data);
//
//  TypeParam gt = TypeParam::FromString(
//      "1.8496745531, 1.9944926680, 0.3201387782, 0.2406420371; 1.1503254469, 1.0055073320, "
//      "0.0751734728, -0.0241974536; 3.6993491062, 3.9889853359, 0.4496530544, 0.6483949073");
//  gt.Reshape({3, 2, 2});
//  TypeParam prediction = g.Evaluate("ScaledDotProductAttention", false);
//
//  ASSERT_TRUE(prediction.AllClose(
//      gt, static_cast<DataType>(5) * fetch::math::function_tolerance<DataType>()));
//}
