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

#include "core/serializers/main_serializer.hpp"
#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "ml/ops/activations/randomised_relu.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class RandomisedReluTest : public ::testing::Test
{
};

TYPED_TEST_CASE(RandomisedReluTest, math::test::TensorFloatingTypes);

TYPED_TEST(RandomisedReluTest, forward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType gt   = TensorType::FromString(
      "1, -0.1508424060836268399, 3, -0.3016848121672536798, 5, -0.4525272182508804919, 7, "
      "-0.6033696243345073595");

  fetch::ml::ops::RandomisedRelu<TensorType> op(DataType{0.03f}, DataType{0.08f}, 12345);
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));

  // Test after generating new random alpha value
  gt = TensorType::FromString(
      "1, -0.1549365367708011032, 3, -0.3098730735416022064, 5, -0.4648096103124032541, 7, "
      "-0.6197461470832044128;");

  prediction = TensorType(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));
  // Test with is_training set to false
  op.SetTraining(false);

  gt = TensorType::FromString("1, -0.11, 3, -0.22, 5, -0.33, 7, -0.44");

  prediction = TensorType(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));
}

TYPED_TEST(RandomisedReluTest, forward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString("1, 3, 5, 7; -2, -4, -6, -8;");
  data.Reshape({2, 2, 2});
  TensorType gt = TensorType::FromString(
      "1, 3, 5, 7; -0.1508424060836268399, -0.3016848121672536798, -0.4525272182508804919, "
      "-0.6033696243345073595;");
  gt.Reshape({2, 2, 2});

  fetch::ml::ops::RandomisedRelu<TensorType> op(DataType{0.03f}, DataType{0.08f}, 12345);
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  EXPECT_TRUE(prediction.AllClose(gt, math::function_tolerance<DataType>(),
                                  math::function_tolerance<DataType>()));
}

TYPED_TEST(RandomisedReluTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data  = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType error = TensorType::FromString("0, 0, 0, 0, 1, 1, 0, 0");
  TensorType gt    = TensorType::FromString("0, 0, 0, 0, 1, 0.0754097138742607365, 0, 0;");
  fetch::ml::ops::RandomisedRelu<TensorType> op(DataType{0.03f}, DataType{0.08f}, 12345);
  std::vector<TensorType>                    prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction[0].AllClose(gt, math::function_tolerance<DataType>(),
                                     math::function_tolerance<DataType>()));

  // Test after generating new random alpha value
  // Forward pass will update random value
  TensorType output(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, output);

  gt         = TensorType::FromString("0, 0, 0, 0, 1, 0.0774682683854005516, 0, 0;");
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction[0].AllClose(gt, math::function_tolerance<DataType>(),
                                     math::function_tolerance<DataType>()));

  // Test with is_training set to false
  op.SetTraining(false);

  gt         = TensorType::FromString("0, 0, 0, 0, 1, 0.055, 0, 0");
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction[0].AllClose(gt, math::function_tolerance<DataType>(),
                                     math::function_tolerance<DataType>()));
}

TYPED_TEST(RandomisedReluTest, backward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString("1, 3, 5, 7;-2, -4, -6, -8;");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("0, 0, 1, 0;0, 0, 1, 0;");
  error.Reshape({2, 2, 2});
  TensorType gt = TensorType::FromString("0, 0, 1, 0;0, 0, 0.0754097138742607365, 0;");
  gt.Reshape({2, 2, 2});

  fetch::ml::ops::RandomisedRelu<TensorType> op(DataType{0.03f}, DataType{0.08f}, 12345);
  std::vector<TensorType>                    prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction[0].AllClose(gt, math::function_tolerance<DataType>(),
                                     math::function_tolerance<DataType>()));
}

TYPED_TEST(RandomisedReluTest, saveparams_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::RandomisedRelu<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::RandomisedRelu<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  fetch::ml::ops::RandomisedRelu<TensorType> op(DataType{0.03f}, DataType{0.08f}, 12345);
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

  // make another prediction with the original graph
  op.Forward(vec_data, prediction);

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
  EXPECT_TRUE(
      new_prediction.AllClose(prediction, static_cast<DataType>(0), static_cast<DataType>(0)));
}

TYPED_TEST(RandomisedReluTest, saveparams_backward_3d_tensor_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::RandomisedRelu<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString("1, 3, 5, 7;-2, -4, -6, -8;");
  data.Reshape({2, 2, 2});
  TensorType error = TensorType::FromString("0, 0, 1, 0;0, 0, 1, 0;");
  error.Reshape({2, 2, 2});

  fetch::ml::ops::RandomisedRelu<TensorType> op(DataType{0.03f}, DataType{0.08f}, 12345);

  // run op once to make sure caches etc. have been filled. Otherwise the test might be trivial!
  std::vector<TensorType> prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<const TensorType>(data)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction =
      new_op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
