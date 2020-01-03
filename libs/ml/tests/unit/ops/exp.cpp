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
#include "ml/ops/exp.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class ExpTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ExpTest, math::test::HighPrecisionTensorFloatingTypes);

TYPED_TEST(ExpTest, forward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data = TensorType::FromString(
      " 0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  TensorType gt = TensorType::FromString(
      "1, 0.135335283236613, 20.0855369231877, 0.018315638888734, 148.413159102577, "
      "0.002478752176666, 1096.63315842846, 0.000335462627903;"
      "0.367879441171442, 7.38905609893065, 0.049787068367864, 54.5981500331442, "
      "0.006737946999085, 403.428793492735, 0.000911881965555, 2980.95798704173");

  fetch::ml::ops::Exp<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(ExpTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString(
      "0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  TensorType gt = TensorType::FromString(
      "1, -0.135335283236613, 40.1710738463753, -0.036631277777468, 445.23947730773, "
      "-0.007436256529999, 4386.53263371383, -0.00134185051161;"
      "1.83939720585721, -36.9452804946533, 0.298722410207184, -327.588900198865, "
      "0.047165628993598, -2824.00155444915, 0.007295055724436, -23847.6638963338");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Exp<TensorType> op;
  std::vector<TensorType>         prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(ExpTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Exp<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Exp<TensorType>;

  TensorType data = TensorType::FromString(
      " 0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  TensorType gt = TensorType::FromString(
      "1, 0.135335283236613, 20.0855369231877, 0.018315638888734, 148.413159102577, "
      "0.002478752176666, 1096.63315842846, 0.000335462627903;"
      "0.367879441171442, 7.38905609893065, 0.049787068367864, 54.5981500331442, "
      "0.006737946999085, 403.428793492735, 0.000911881965555, 2980.95798704173");

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
}

TYPED_TEST(ExpTest, saveparams_backward_test)
{
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::Exp<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data = TensorType::FromString(
      "0, -2, 3,-4, 5,-6, 7,-8;"
      "-1,  2,-3, 4,-5, 6,-7, 8");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Exp<TensorType> op;
  std::vector<TensorType>         prediction =
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
