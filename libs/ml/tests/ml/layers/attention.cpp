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
#include "ml/layers/fully_connected.hpp"
#include "ml/layers/attention.hpp"

#include "gtest/gtest.h"

template <typename T>
class SelfAttentionTest : public ::testing::Test
{
};

// TODO (private 507)
using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>>;

TYPED_TEST_CASE(SelfAttentionTest, MyTypes);

TYPED_TEST(SelfAttentionTest, output_shape_test)  // Use the class as a Node
{
	using DataType = typename TypeParam::Type;
  fetch::ml::Graph<TypeParam> g;

  std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
	std::string key = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
	std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
  g.template AddNode<fetch::ml::layers::Attention<TypeParam>>("SelfAttention", {query, key, value}, 3u,
                                                                  3u, DataType(0.1));
  
  TypeParam query_data = TypeParam({7, 4, 1});
	TypeParam key_data = TypeParam({5, 4, 1});
	TypeParam value_data = TypeParam({5, 3, 1});
  query_data.Fill(static_cast<DataType>(1));
	key_data.Fill(static_cast<DataType>(1));
	value_data.Fill(static_cast<DataType>(1));
  
  g.SetInput(query, query_data);
	g.SetInput(key, key_data);
	g.SetInput(value, value_data);

  TypeParam prediction = g.Evaluate("SelfAttention", false);
  ASSERT_EQ(prediction.shape()[0], 7);
  ASSERT_EQ(prediction.shape()[1], 3);
	ASSERT_EQ(prediction.shape()[2], 1);
}
