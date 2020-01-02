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
#include "ml/ops/placeholder.hpp"
#include "ml/ops/reduce_mean.hpp"
#include "ml/serializers/ml_types.hpp"
#include "ml/utilities/graph_builder.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class ReduceMeanTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ReduceMeanTest, math::test::TensorFloatingTypes);

TYPED_TEST(ReduceMeanTest, forward_2_2_2_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  TensorType gt = TensorType::FromString("2.5, 5, 0, 400");
  gt.Reshape({2, 1, 2});

  fetch::ml::ops::ReduceMean<TypeParam> op(1);

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  ASSERT_EQ(prediction.shape().size(), 3);
  ASSERT_EQ(prediction.shape().at(0), 2);
  ASSERT_EQ(prediction.shape().at(1), 1);
  ASSERT_EQ(prediction.shape().at(2), 2);

  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(ReduceMeanTest, backward_2_2_2_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("1, -2, -1, 2");
  error.Reshape({2, 1, 2});
  TensorType gt_error = TensorType::FromString("0.5, -1, 0.5, -1, -0.5, 1, -0.5, 1");
  gt_error.Reshape({2, 2, 2});

  auto shape = error.shape();

  fetch::ml::ops::ReduceMean<TypeParam> op(1);

  std::vector<TensorType> error_signal =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  ASSERT_EQ(error_signal.at(0).shape().size(), 3);
  ASSERT_EQ(error_signal.at(0).shape().at(0), 2);
  ASSERT_EQ(error_signal.at(0).shape().at(1), 2);
  ASSERT_EQ(error_signal.at(0).shape().at(2), 2);

  ASSERT_TRUE(error_signal.at(0).AllClose(gt_error, fetch::math::function_tolerance<DataType>(),
                                          fetch::math::function_tolerance<DataType>()));
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(ReduceMeanTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::ReduceMean<TensorType>::SPType;
  using OpType        = fetch::ml::ops::ReduceMean<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  OpType op(1);

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

TYPED_TEST(ReduceMeanTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using OpType     = fetch::ml::ops::ReduceMean<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("1, -2, -1, 2");
  error.Reshape({2, 1, 2});

  fetch::ml::ops::ReduceMean<TypeParam> op(1);

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

TYPED_TEST(ReduceMeanTest, ReduceMean_graph_serialization_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = fetch::ml::GraphSaveableParams<TensorType>;

  TensorType data = TensorType::FromString("1, 2, 4, 8, 100, 1000, -100, -200");
  data.Reshape({2, 2, 2});

  fetch::ml::Graph<TensorType> g;

  std::string input_name = g.template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>("Input", {});
  std::string output_name =
      g.template AddNode<fetch::ml::ops::ReduceMean<TensorType>>("Output", {input_name}, 1);

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
