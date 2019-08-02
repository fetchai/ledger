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
#include <ml/layers/multihead_attention.hpp>

#include "ml/layers/fully_connected.hpp"

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
  std::string key   = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Key", {});
  std::string value = g.template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Value", {});
  g.template AddNode<fetch::ml::layers::MultiheadAttention<TypeParam>>(
      "ScaledDotProductAttention", {query, key, value}, static_cast<SizeType>(4),
      static_cast<SizeType>(12), DataType(0.1));
  TypeParam query_data = TypeParam({12, 25, 4});
  TypeParam key_data   = query_data;
  TypeParam value_data = query_data;
  g.SetInput(query, query_data);
  g.SetInput(key, key_data);
  g.SetInput(value, value_data);

  TypeParam prediction = g.Evaluate("ScaledDotProductAttention", false);
  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape()[0], 12);
  ASSERT_EQ(prediction.shape()[1], 25);
  ASSERT_EQ(prediction.shape()[2], 4);
}

TYPED_TEST(MultiheadAttention, trivial_test)
{
  //	using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  TypeParam data1 = TypeParam::UniformRandom(100u);
  TypeParam gt    = TypeParam::UniformRandom(100u);

  data1.Reshape({10, 10});
  gt.Reshape({10, 10});

  auto g = std::make_shared<fetch::ml::Graph<TypeParam>>();

  std::string in1 = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("in1", {});
  std::string fc1 =
      g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>("fc1", {in1}, 10, 10);
  std::string fc3 =
      g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>("fc3", {fc1}, 10, 10);
  std::string fc4 =
      g->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>("fc4", {fc1}, 10, 10);
  std::string sum = g->template AddNode<fetch::ml::ops::Add<TypeParam>>("add", {fc3, fc4});

  std::string label = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("label", {});
  std::string error =
      g->template AddNode<fetch::ml::ops::MeanSquareErrorLoss<TypeParam>>("mse", {sum, label});

  auto optimiser = fetch::ml::optimisers::SGDOptimiser<TypeParam>(g, {in1}, label, error,
                                                                  static_cast<DataType>(0.9));

  optimiser.Run({data1}, gt);
}

TYPED_TEST(MultiheadAttention, backward_test)  // Use the class as an Ops
{
  using SizeType = typename TypeParam::SizeType;
  using DataType = typename TypeParam::Type;

  fetch::ml::layers::MultiheadAttention<TypeParam> m_att(static_cast<SizeType>(4),
                                                         static_cast<SizeType>(12), DataType(0.9));
  TypeParam input_data(std::vector<typename TypeParam::SizeType>({12, 20, 5}));
  TypeParam output(m_att.ComputeOutputShape({std::make_shared<TypeParam>(input_data)}));
  m_att.Forward({std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(input_data),
                 std::make_shared<TypeParam>(input_data)},
                output);

  TypeParam error_signal(std::vector<typename TypeParam::SizeType>({16, 20, 5}));

  std::vector<TypeParam> backprop_error = m_att.Backward(
      {std::make_shared<TypeParam>(input_data), std::make_shared<TypeParam>(input_data),
       std::make_shared<TypeParam>(input_data)},
      error_signal);

  // check there are proper number of error signals, this is an indirect test for subgraph backward
  // signal pass
  ASSERT_EQ(backprop_error.size(), 3);

  // check all shape are the same
  bool                  all_same_shape = true;
  std::vector<SizeType> prev_shape;
  for (auto error : backprop_error)
  {
    auto shape = error.shape();
    if (prev_shape.size() == 0)
    {
      prev_shape = shape;
      continue;
    }
    else
    {
      if (shape != prev_shape)
      {
        all_same_shape = false;
        break;
      }
      else
      {
        prev_shape = shape;
      }
    }
  }
  ASSERT_TRUE(all_same_shape);

  ASSERT_EQ(backprop_error[0].shape().size(), 3);
  ASSERT_EQ(backprop_error[0].shape()[0], 16);
  ASSERT_EQ(backprop_error[0].shape()[1], 20);
  ASSERT_EQ(backprop_error[0].shape()[2], 5);
}
