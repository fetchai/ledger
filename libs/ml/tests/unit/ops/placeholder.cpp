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

#include "core/serializers/main_serializer_definition.hpp"
#include "math/base_types.hpp"
#include "ml/core/graph.hpp"
#include "ml/layers/fully_connected.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class PlaceholderAllTest : public ::testing::Test
{
};

template <typename T>
class PlaceholderNonIntTest : public ::testing::Test
{
};

TYPED_TEST_CASE(PlaceholderAllTest, math::test::TensorIntAndFloatingTypes);
TYPED_TEST_CASE(PlaceholderNonIntTest, math::test::TensorFloatingTypes);

TYPED_TEST(PlaceholderAllTest, set_data)
{
  TypeParam data = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");
  TypeParam gt   = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  fetch::ml::ops::PlaceHolder<TypeParam> op;
  op.SetData(data);

  TypeParam prediction(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(PlaceholderAllTest, mutable_test)
{
  TypeParam data = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");
  TypeParam gt   = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  fetch::ml::ops::PlaceHolder<TypeParam> op;
  op.SetData(data);

  TypeParam prediction(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));

  // reset
  data = TypeParam::FromString("12, 13, -14, 15, 16, -17, 18, 19");
  gt   = TypeParam::FromString("12, 13, -14, 15, 16, -17, 18, 19");

  op.SetData(data);

  prediction = TypeParam(op.ComputeOutputShape({}));
  op.Forward({}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt));
}

TYPED_TEST(PlaceholderAllTest, trainable_test)
{
  TypeParam data = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  auto g = std::make_shared<fetch::ml::Graph<TypeParam>>();
  g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("PlaceHolder", {});
  g->SetInput("PlaceHolder", data);

  auto prediction1 = g->Evaluate("PlaceHolder");
  g->BackPropagate("PlaceHolder");
  auto grads = g->GetGradients();
  g->ApplyGradients(grads);
  auto prediction2 = g->Evaluate("PlaceHolder");

  // tests that the placeholder does not update after training
  EXPECT_TRUE(prediction1.AllClose(prediction2));
}

TYPED_TEST(PlaceholderAllTest, shareable_test)
{
  TypeParam data = TypeParam::FromString("1, 2, 3, 4, 5, 6, 7, 8");

  auto g      = std::make_shared<fetch::ml::Graph<TypeParam>>();
  auto name_1 = g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("PlaceHolder", {});

  // cannot share placeholder node
  EXPECT_ANY_THROW(g->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("PlaceHolder", {}));
}

// tests that sharing a layer with placeholders is fine, even though you can't share a placeholder
TYPED_TEST(PlaceholderNonIntTest, shareable_layer_with_placeholder)
{
  TypeParam data = TypeParam::FromString("1; 2");

  auto graph = std::make_shared<fetch::ml::Graph<TypeParam>>();
  auto placeholder_name =
      graph->template AddNode<fetch::ml::ops::PlaceHolder<TypeParam>>("Input", {});
  auto layer1_name = graph->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {placeholder_name}, 2, 2);
  auto layer2_name = graph->template AddNode<fetch::ml::layers::FullyConnected<TypeParam>>(
      "FC1", {placeholder_name}, 2, 2);

  graph->SetInput(placeholder_name, data);

  auto prediction1 = graph->Evaluate(layer1_name);
  auto prediction2 = graph->Evaluate(layer2_name);

  EXPECT_TRUE(prediction1.AllClose(prediction2));
}

TYPED_TEST(PlaceholderAllTest, saveable_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::PlaceHolder<TensorType>::SPType;
  using OpType     = typename fetch::ml::ops::PlaceHolder<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType gt   = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  OpType op;
  op.SetData(data);

  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));

  op.Forward({}, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::static_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // placeholders do not store their data in serialisation, so we re set the data here
  new_op.SetData(data);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward({}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
