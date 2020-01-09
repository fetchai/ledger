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
#include "ml/ops/activations/gelu.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include <vector>

namespace fetch {
namespace ml {
namespace test {

template <typename T>
class GeluTest : public ::testing::Test
{
};

TYPED_TEST_CASE(GeluTest, math::test::HighPrecisionTensorFloatingTypes);

TYPED_TEST(GeluTest, forward_test_3d)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType data = TensorType::FromString("-10, -2, -1, -0.5, 0, 0.2, 1.6, 5.7, 12");
  data.Reshape({3, 1, 3});
  TensorType gt = TensorType::FromString(
      "-0.0000000000, -0.0454022884, -0.1588079929, -0.1542859972, 0.0000000000,  0.1158514246,  "
      "1.5121370554,  5.6999998093, 12.0000000000");
  gt.Reshape({3, 1, 3});

  fetch::ml::ops::Gelu<TensorType> op;
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);
  // test correct values
  ASSERT_TRUE(prediction.AllClose(
      gt, fetch::math::function_tolerance<DataType>(),
      fetch::math::Type<DataType>("2.8") * fetch::math::function_tolerance<DataType>()));

  // gelu can overflow for some fixed point types for these data
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(GeluTest, backward_3d_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TensorType::Type;

  TensorType data = TensorType::FromString("-1.1, -0.4, -0.5, -0.2, 0, 0.2, 1.6, 1.7, 2");
  data.Reshape({3, 1, 3});
  TensorType error_signal = TensorType::FromString("-3, 2, 3, 4.5, 0.2, 6.6, 7.1, 10, 0.02");
  error_signal.Reshape({3, 1, 3});
  TensorType gt = TensorType::FromString(
      "0.3109784424,  0.3946822584,  0.3978902698,  1.5414382219, 0.1000000015,  4.3392238617,  "
      "7.9740133286, 11.1591463089, 0.0217219833");
  gt.Reshape({3, 1, 3});

  fetch::ml::ops::Gelu<TensorType> op;
  std::vector<TensorType>          prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error_signal);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     DataType{100} * fetch::math::function_tolerance<DataType>()));

  // gelu can overflow for some fixed point types for these data
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(GeluTest, saveparams_test)
{
  using DataType      = typename TypeParam::Type;
  using TensorType    = TypeParam;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Gelu<TensorType>::SPType;

  TensorType data = TensorType::FromString("-10, -2, -1, -0.5, 0, 0.2, 1.6, 5.7, 12");
  data.Reshape({3, 1, 3});
  TensorType gt = TensorType::FromString(
      "-0.0000000000, -0.0454022884, -0.1588079929, -0.1542859972, 0.0000000000,  0.1158514246,  "
      "1.5121370554,  5.6999998093, 12.0000000000");
  gt.Reshape({3, 1, 3});

  fetch::ml::ops::Gelu<TensorType> op;
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

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
  fetch::ml::ops::Gelu<TensorType> new_op(*dsp2);

  // check that new predictions match the old
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  new_op.Forward(VecTensorType({std::make_shared<const TensorType>(data)}), new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));

  // gelu can overflow for some fixed point types for these data
  fetch::math::state_clear<DataType>();
}

TYPED_TEST(GeluTest, saveparams_backward_3d_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::Gelu<TensorType>;
  using SPType     = typename fetch::ml::ops::Gelu<TensorType>::SPType;

  TensorType data = TensorType::FromString("-1.1, -0.4, -0.5, -0.2, 0, 0.2, 1.6, 1.7, 2");
  data.Reshape({3, 1, 3});
  TensorType error = TensorType::FromString("-3, 2, 3, 4.5, 0.2, 6.6, 7.1, 10, 0.02");
  error.Reshape({3, 1, 3});
  TensorType gt = TensorType::FromString(
      "0.3109784424,  0.3946822584,  0.3978902698,  1.5414382219, 0.1000000015,  4.3392238617,  "
      "7.9740133286, 11.1591463089, 0.0217219833");
  gt.Reshape({3, 1, 3});

  fetch::ml::ops::Gelu<TensorType> op;

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

  // gelu can overflow for some fixed point types for these data
  fetch::math::state_clear<DataType>();
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
