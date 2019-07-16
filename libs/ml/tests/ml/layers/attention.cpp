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

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::Attention<TypeParam>>("SelfAttention", {"Input", "Input", "Input"}, 5u,
                                                                  5u, DataType(0.1));

  
  TypeParam data = TypeParam({5, 3, 1});
  data.Fill(1);
//  TypeParam tmp_data = TypeParam::FromString("1, 2, 3, 4, 5; 6, 7, 8, 9, 10; 11, 12, 13, 14, 15");
//
//  auto data_view = data.View();
//  auto tmp_data_view = tmp_data.View();
//  data_view.Assign(tmp_data_view);
//
  auto data_it = data.begin();
  while (data_it.is_valid())
  {
  	std::cout << "*data_it: " << *data_it << std::endl;
  	++data_it;
  }
  
  
  
  
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("SelfAttention", true);
  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 42);
  ASSERT_EQ(prediction.shape()[1], 1);
}
