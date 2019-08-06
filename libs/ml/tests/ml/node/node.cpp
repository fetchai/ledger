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
#include "ml/core/node.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/activations/relu.hpp"
#include "ml/ops/placeholder.hpp"

#include "gtest/gtest.h"
template <typename T>
class NodeTest : public ::testing::Test
{
};

using MyTypes =
    ::testing::Types<fetch::math::Tensor<int32_t>, fetch::math::Tensor<float>,
                     fetch::math::Tensor<double>, fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                     fetch::math::Tensor<fetch::fixed_point::fp64_t>>;

TYPED_TEST_CASE(NodeTest, MyTypes);

TYPED_TEST(NodeTest, node_placeholder)
{
  fetch::ml::Node<TypeParam> placeholder(fetch::ml::OpType::OP_PLACEHOLDER, "PlaceHolder");
  TypeParam                  data(std::vector<std::uint64_t>({5, 5}));
  std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder.GetOp())
      ->SetData(data);

  auto placeholder_ptr =
      std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder.GetOp());
  TypeParam output(placeholder_ptr->ComputeOutputShape({}));
  placeholder_ptr->Forward({}, output);

  EXPECT_EQ(output, data);
  EXPECT_EQ(*placeholder.Evaluate(true), data);
}

TYPED_TEST(NodeTest, node_relu)
{
  auto placeholder = std::make_shared<fetch::ml::Node<TypeParam>>(fetch::ml::OpType::OP_PLACEHOLDER,
                                                                  "PlaceHolder");
  auto placeholder_ptr =
      std::dynamic_pointer_cast<fetch::ml::ops::PlaceHolder<TypeParam>>(placeholder->GetOp());

  auto relu = std::make_shared<fetch::ml::Node<TypeParam>>(
      fetch::ml::OpType::OP_RELU, "Relu",
      []() { return std::make_shared<fetch::ml::ops::Relu<TypeParam>>(); });

  relu->AddInput(placeholder);

  auto data =
      TypeParam::FromString("0, -1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12, -13, 14, -15, 16");
  data.Reshape({4, 4});
  auto gt = TypeParam::FromString("0, 0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12, 0, 14, 0, 16");
  gt.Reshape({4, 4});

  placeholder_ptr->SetData(data);
  relu->ResetCache(true);

  TypeParam output(placeholder_ptr->ComputeOutputShape({}));
  placeholder_ptr->Forward({}, output);

  EXPECT_EQ(output, data);
  EXPECT_EQ(*(placeholder->Evaluate(true)), data);
  EXPECT_TRUE(relu->Evaluate(true)->Copy().AllClose(gt));
}
