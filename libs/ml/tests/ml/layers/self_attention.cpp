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

#include "ml/layers/self_attention.hpp"
#include "math/tensor.hpp"
#include "ml/layers/fully_connected.hpp"
#include <gtest/gtest.h>

template <typename T>
class SelfAttentionTest : public ::testing::Test
{
};

// TODO (private 507)
using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>>;
TYPED_TEST_CASE(SelfAttentionTest, MyTypes);

TYPED_TEST(SelfAttentionTest, output_shape_test)  // Use the class as a Node
{
  fetch::ml::Graph<TypeParam> g;

  g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  g.template AddNode<fetch::ml::layers::SelfAttention<TypeParam>>("SelfAttention", {"Input"}, 50u,
                                                                  42u, 10u);

  std::shared_ptr<TypeParam> data =
      std::make_shared<TypeParam>(std::vector<typename TypeParam::SizeType>({5, 10}));
  g.SetInput("Input", data);

  TypeParam prediction = g.Evaluate("SelfAttention");
  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape()[0], 1);
  ASSERT_EQ(prediction.shape()[1], 42);
}
