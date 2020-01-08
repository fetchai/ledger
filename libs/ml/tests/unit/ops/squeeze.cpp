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
#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "ml/core/graph.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/squeeze.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SqueezeTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SqueezeTest, math::test::TensorFloatingTypes);

TYPED_TEST(SqueezeTest, forward_1_6_1_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({1, 6, 1});

  fetch::ml::ops::Squeeze<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  ASSERT_EQ(prediction.shape().size(), 2);
  ASSERT_EQ(prediction.shape().at(0), 1);
  ASSERT_EQ(prediction.shape().at(1), 6);

  ASSERT_TRUE(prediction.AllClose(data, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  TypeParam prediction2(op.ComputeOutputShape({std::make_shared<const TensorType>(prediction)}));
  op.Forward({std::make_shared<const TensorType>(prediction)}, prediction2);

  ASSERT_EQ(prediction2.shape().size(), 1);
  ASSERT_EQ(prediction2.shape().at(0), 6);

  ASSERT_TRUE(prediction2.AllClose(data, fetch::math::function_tolerance<DataType>(),
                                   fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SqueezeTest, forward_throw_test)
{
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6});

  fetch::ml::ops::Squeeze<TypeParam> op;

  EXPECT_ANY_THROW(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  TypeParam prediction({6});
  EXPECT_ANY_THROW(op.Forward({std::make_shared<const TensorType>(data)}, prediction));
}

TYPED_TEST(SqueezeTest, backward_1_5_1_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, -2, 4, -10, 100");
  data.Reshape({1, 5, 1});
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");
  error.Reshape({1, 5});

  auto shape = error.shape();

  fetch::ml::ops::Squeeze<TypeParam> op;

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_EQ(error_signal.at(0).shape().size(), 3);
  ASSERT_EQ(error_signal.at(0).shape().at(0), 1);
  ASSERT_EQ(error_signal.at(0).shape().at(1), 5);
  ASSERT_EQ(error_signal.at(0).shape().at(2), 1);

  ASSERT_TRUE(error_signal.at(0).AllClose(error, fetch::math::function_tolerance<DataType>(),
                                          fetch::math::function_tolerance<DataType>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqueezeTest, backward_1_5_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, -2, 4, -10, 100");
  data.Reshape({1, 5});
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");
  error.Reshape({5});

  auto shape = error.shape();

  fetch::ml::ops::Squeeze<TypeParam> op;

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_EQ(error_signal.at(0).shape().size(), 2);
  ASSERT_EQ(error_signal.at(0).shape().at(0), 1);
  ASSERT_EQ(error_signal.at(0).shape().at(1), 5);

  ASSERT_TRUE(error_signal.at(0).AllClose(error, fetch::math::function_tolerance<DataType>(),
                                          fetch::math::function_tolerance<DataType>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqueezeTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Squeeze<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Squeeze<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6, 1});

  OpType op;

  TensorType    prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  VecTensorType vec_data({std::make_shared<const TensorType>(data)});

  op.Forward(vec_data, prediction);

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

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqueezeTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::Squeeze<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString("1, -2, 4, -10, 100");
  data.Reshape({1, 5});
  TensorType error = TensorType::FromString("1, 1, 1, 2, 0");
  error.Reshape({5});

  fetch::ml::ops::Squeeze<TypeParam> op;

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // get another error_signal with the original op
  error_signal = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new error_signal match the old
  std::vector<TensorType> new_error_signal =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(error_signal.at(0).AllClose(
      new_error_signal.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(SqueezeTest, squeeze_graph_serialization_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = fetch::ml::GraphSaveableParams<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000");
  data.Reshape({6, 1});

  fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::Squeeze<TensorType>>("Output", {input_name});

  g.SetInput(input_name, data);
  TypeParam output = g.Evaluate("Output");

  // extract saveparams
  SPType gsp = g.GetGraphSaveableParams();

  fetch::serializers::MsgPackSerializer b;
  b << gsp;

  // deserialize
  b.seek(0);
  SPType dsp2;
  b >> dsp2;

  // rebuild graph
  auto new_graph_ptr = std::make_shared<fetch::ml::Graph<TensorType>>();
  fetch::ml::utilities::BuildGraph(gsp, new_graph_ptr);

  new_graph_ptr->SetInput(input_name, data);
  TypeParam output2 = new_graph_ptr->Evaluate("Output");

  // Test correct values
  ASSERT_EQ(output.shape(), output2.shape());
  ASSERT_TRUE(output.AllClose(output2, fetch::math::function_tolerance<DataType>(),
                              fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
