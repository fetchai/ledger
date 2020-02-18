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

#include "test_types.hpp"

#include "ml/layers/multihead_attention.hpp"
#include "ml/ops/loss_functions/mean_square_error_loss.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class MultiheadAttention : public ::testing::Test
{
};

// float32 tends to overflow here
TYPED_TEST_SUITE(MultiheadAttention, math::test::HighPrecisionTensorFloatingTypes, );

TYPED_TEST(MultiheadAttention, input_output_dimension_check)  // Use the class as a subgraph
{
  using DataType = typename TypeParam::Type;
  using SizeType = fetch::math::SizeType;

  fetch::ml::Graph<TypeParam> g;

  std::string query = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Query", {});
  std::string key   = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
  std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
  std::string mask  = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Mask", {});
  g.template AddNode<fetch::ml::layers::MultiheadAttention<TypeParam>>(
      "MultiheadAttention", {query, key, value, mask}, static_cast<SizeType>(4),
      static_cast<SizeType>(12), fetch::math::Type<DataType>("0.1"));
  TypeParam query_data = TypeParam({12, 25, 4});
  query_data.Fill(DataType{0});
  TypeParam key_data   = query_data;
  TypeParam value_data = query_data;
  TypeParam mask_data  = TypeParam({25, 25, 4});
  mask_data.Fill(DataType{1});
  g.SetInput(query, query_data);
  g.SetInput(key, key_data);
  g.SetInput(value, value_data);
  g.SetInput(mask, mask_data);
  g.Compile();

  TypeParam prediction = g.Evaluate("MultiheadAttention", false);
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 12);
  ASSERT_EQ(prediction.shape()[1], 25);
  ASSERT_EQ(prediction.shape()[2], 4);
}

TYPED_TEST(MultiheadAttention, backward_test)  // Use the class as an Ops
{
  using SizeType = fetch::math::SizeType;
  using DataType = typename TypeParam::Type;
  fetch::ml::layers::MultiheadAttention<TypeParam> m_att(
      static_cast<SizeType>(4), static_cast<SizeType>(12), fetch::math::Type<DataType>("0.9"));
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({12, 20, 5}));

  TypeParam mask_data = TypeParam({20, 20, 5});
  mask_data.Fill(DataType{1});
  TypeParam output(m_att.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  m_att.Forward({std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(input_data),
                 std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(mask_data)},
                output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({12, 20, 5}));

  std::vector<TypeParam> backprop_error = m_att.Backward(
      {std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(input_data),
       std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(mask_data)},
      error_signal);

  // check there are proper number of error signals, this is an indirect test for subgraph backward
  // signal pass
  ASSERT_EQ(backprop_error.size(), 4);

  // check all shape are the same apart from the mask error signal
  bool                  all_same_shape = true;
  std::vector<SizeType> prev_shape;
  for (SizeType i = 0; i < backprop_error.size() - 1; i++)
  {
    auto           error = backprop_error.at(i);
    decltype(auto) shape = error.shape();
    if (prev_shape.empty())
    {
      prev_shape = shape;
      continue;
    }

    if (shape != prev_shape)
    {
      all_same_shape = false;
      break;
    }

    prev_shape = shape;
  }
  ASSERT_TRUE(all_same_shape);

  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 12);
  ASSERT_EQ(backprop_error[0].shape()[1], 20);
  ASSERT_EQ(backprop_error[0].shape()[2], 5);
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
