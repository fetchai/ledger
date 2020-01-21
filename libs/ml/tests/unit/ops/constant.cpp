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

#include "ml/core/graph.hpp"
#include "ml/ops/constant.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

namespace {
template <typename T>
class ConstantTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ConstantTest, fetch::math::test::TensorIntAndFloatingTypes);

TYPED_TEST(ConstantTest, set_data)
{
  TypeParam data = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");
  TypeParam gt   = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  fetch::ml::ops::Constant<TypeParam> op;
  op.SetData(data);

  TypeParam prediction(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(ConstantTest, mutable_test)
{
  TypeParam data = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");
  TypeParam gt   = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  fetch::ml::ops::Constant<TypeParam> op;
  op.SetData(data);

  TypeParam prediction(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt));

  // constants are mutable, and therefore cannot be set twice
  EXPECT_ANY_THROW(op.SetData(data));
}

TYPED_TEST(ConstantTest, trainable_test)
{
  TypeParam data = TypeParam::FromString("1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0");

  auto g = std::make_shared<fetch::ml::Graph<TypeParam>>();
  g->template AddNode<fetch::ml::ops::Constant<TypeParam>>("Constant", {});
  g->SetInput("Constant", data);

  auto prediction1 = g->Evaluate("Constant");
  g->BackPropagate("Constant");
  auto grads = g->GetGradients();
  g->ApplyGradients(grads);
  auto prediction2 = g->Evaluate("Constant");

  // tests that the constant does not update after training
  EXPECT_TRUE(prediction1.AllClose(prediction2));
}

TYPED_TEST(ConstantTest, shareable_test)
{
  TypeParam data = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  auto g      = std::make_shared<fetch::ml::Graph<TypeParam>>();
  auto name_1 = g->template AddNode<fetch::ml::ops::Constant<TypeParam>>("Constant", {});
  auto name_2 = g->template AddNode<fetch::ml::ops::Constant<TypeParam>>("Constant", {});
  g->SetInput(name_1, data);

  auto prediction1_node1 = g->Evaluate(name_1);
  auto prediction1_node2 = g->Evaluate(name_2);

  // tests that the constant does not update after training
  EXPECT_TRUE(prediction1_node1.AllClose(prediction1_node2));
}

}  // namespace
