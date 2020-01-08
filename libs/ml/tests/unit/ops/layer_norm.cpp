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

#include "core/serializers/main_serializer.hpp"
#include "ml/ops/layer_norm.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class LayerNormTest : public ::testing::Test
{
};

TYPED_TEST_CASE(LayerNormTest, math::test::TensorFloatingTypes);

TYPED_TEST(LayerNormTest, forward_test_2d)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString(
      "1, 2;"
      "2, 3;"
      "3, 6");

  TensorType gt = TensorType::FromString(
      "-1.2247448, -0.98058067;"
      "0, -0.39223227;"
      "1.22474487, 1.372812945");

  fetch::ml::ops::LayerNorm<TensorType> op(static_cast<typename TypeParam::SizeType>(0));

  TensorType prediction(op.ComputeOutputShape({std::make_shared<TensorType>(data)}));
  op.Forward({std::make_shared<TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(LayerNormTest, forward_test_3d)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString(
      "1, 2, 3, 0;"
      "2, 3, 2, 1;"
      "3, 6, 4, 13");

  data.Reshape({3, 2, 2});
  auto s1 = data.View(0).ToString();
  auto s2 = data.View(1).ToString();

  TensorType gt = TensorType::FromString(
      "-1.22474487, -0.98058068, 0, -0.79006571;"
      "0, -0.39223227, -1.22474487,  -0.62076591;"
      "1.22474487,  1.37281295, 1.22474487, 1.41083162");

  fetch::ml::ops::LayerNorm<TensorType> op(static_cast<typename TypeParam::SizeType>(0));

  TensorType prediction(op.ComputeOutputShape({std::make_shared<TensorType>(data)}));
  op.Forward({std::make_shared<TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(
      prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                          static_cast<DataType>(5) * fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(LayerNormTest, backward_test_2d)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString(
      "1, 1;"
      "2, 0;"
      "1, 1");

  TensorType error_signal = TensorType::FromString(
      "-1, 2;"
      "2, 0;"
      "1, 1");

  TensorType gt = TensorType::FromString(
      "-2.12132050, 1.06066041;"
      "0.000001272, -0.00000095;"
      "2.12131923, -1.06065946");

  fetch::ml::ops::LayerNorm<TensorType> op(static_cast<typename TypeParam::SizeType>(0));

  auto backward_errors = op.Backward({std::make_shared<TensorType>(data)}, error_signal).at(0);

  // test correct values
  ASSERT_TRUE(backward_errors.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                       DataType{15} * fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(LayerNormTest, backward_test_3d)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString(
      "1, 1, 0.5, 2;"
      "2, 0, 3, 1;"
      "1, 1, 7, 9");
  data.Reshape({3, 2, 2});

  TensorType error_signal = TensorType::FromString(
      "-1, 2, 1, 1;"
      "2, 0, 1, 3;"
      "1, 1, 1, 6");
  error_signal.Reshape({3, 2, 2});

  TensorType gt = TensorType::FromString(
      "-2.12132050, 1.06066041, 0, -0.374634325;"
      "0.000001272, -0.00000095, 0, 0.327805029;"
      "2.12131923, -1.06065946, 0, 0.0468292959");
  gt.Reshape({3, 2, 2});

  fetch::ml::ops::LayerNorm<TensorType> op(static_cast<typename TypeParam::SizeType>(0));

  TensorType prediction(op.ComputeOutputShape({std::make_shared<TensorType>(data)}));
  auto backward_errors = op.Backward({std::make_shared<TensorType>(data)}, error_signal).at(0);

  // test correct values
  ASSERT_TRUE(backward_errors.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                       DataType{15} * fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(LayerNormTest, saveparams_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;
  using SPType     = typename fetch::ml::ops::LayerNorm<TensorType>::SPType;
  using OpType     = fetch::ml::ops::LayerNorm<TensorType>;

  TensorType data = TensorType::FromString(
      "1, 2, 3, 0;"
      "2, 3, 2, 1;"
      "3, 6, 4, 13");

  data.Reshape({3, 2, 2});
  auto s1 = data.View(0).ToString();
  auto s2 = data.View(1).ToString();

  TensorType gt = TensorType::FromString(
      "-1.22474487, -0.98058068, 0, -0.79006571;"
      "0, -0.39223227, -1.22474487,  -0.62076591;"
      "1.22474487,  1.37281295, 1.22474487, 1.41083162");

  fetch::ml::ops::LayerNorm<TensorType> op(static_cast<typename TypeParam::SizeType>(0));

  TensorType prediction(op.ComputeOutputShape({std::make_shared<TensorType>(data)}));
  op.Forward({std::make_shared<TensorType>(data)}, prediction);

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
  TensorType new_prediction(op.ComputeOutputShape({std::make_shared<TensorType>(data)}));
  op.Forward({std::make_shared<TensorType>(data)}, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(LayerNormTest, saveparams_backward_test_3d)
{
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::LayerNorm<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString(
      "1, 1, 0.5, 2;"
      "2, 0, 3, 1;"
      "1, 1, 7, 9");
  data.Reshape({3, 2, 2});

  TensorType error_signal = TensorType::FromString(
      "-1, 2, 1, 1;"
      "2, 0, 1, 3;"
      "1, 1, 1, 6");
  error_signal.Reshape({3, 2, 2});

  fetch::ml::ops::LayerNorm<TensorType> op(static_cast<typename TypeParam::SizeType>(0));

  auto prediction = op.Backward({std::make_shared<TensorType>(data)}, error_signal);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward({std::make_shared<TensorType>(data)}, error_signal);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  auto new_prediction = new_op.Backward({std::make_shared<TensorType>(data)}, error_signal);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
