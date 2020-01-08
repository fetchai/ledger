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
#include "ml/ops/multiply.hpp"
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
class MultiplyTest : public ::testing::Test
{
};

TYPED_TEST_CASE(MultiplyTest, math::test::TensorFloatingTypes);

TYPED_TEST(MultiplyTest, forward_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType gt = TensorType::FromString(
      "8, 14, 18,20, 20,18, 14,8;"
      "-8,  14,-18, 20,-20, 18,-14, 8");

  fetch::ml::ops::Multiply<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}));
  op.Forward({std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)},
             prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));

  ASSERT_TRUE(!math::state_overflow<typename TypeParam::Type>());
}

TYPED_TEST(MultiplyTest, backward_test_NMB_N11)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, 2, 5, 6;"
      "3, 4, 7, 8");
  data_1.Reshape({2, 2, 2});

  TensorType data_2 = TensorType::FromString("1, -1");
  data_2.Reshape({2, 1, 1});

  TensorType error = TensorType::FromString(
      "0, 1, 4, 5;"
      "2, 3, 6, 7");
  error.Reshape({2, 2, 2});

  TensorType gt_1 = TensorType::FromString(
      "0, 1, 4, 5;"
      "-2, -3, -6, -7");
  gt_1.Reshape({2, 2, 2});

  TensorType gt_2 = TensorType::FromString(
      "52;"
      "116");
  gt_2.Reshape({2, 1, 1});

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values and shape
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[0].shape() == data_1.shape());
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].shape() == data_2.shape());
}

TYPED_TEST(MultiplyTest, backward_test_NMB_111)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, 2, 5, 6;"
      "3, 4, 7, 8");
  data_1.Reshape({2, 2, 2});

  TensorType data_2 = TensorType::FromString("-1");
  data_2.Reshape({1, 1, 1});

  TensorType error = TensorType::FromString(
      "0, 1, 4, 5;"
      "2, 3, 6, 7");
  error.Reshape({2, 2, 2});

  TensorType gt_1 = TensorType::FromString(
      "0, -1, -4, -5;"
      "-2, -3, -6, -7");
  gt_1.Reshape({2, 2, 2});

  TensorType gt_2 = TensorType::FromString("168");
  gt_2.Reshape({1, 1, 1});

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values and shape
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[0].shape() == data_1.shape());
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].shape() == data_2.shape());
}

TYPED_TEST(MultiplyTest, backward_test_NB_N1)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, 2, 5, 6;"
      "3, 4, 7, 8");

  TensorType data_2 = TensorType::FromString("1, -1");
  data_2.Reshape({2, 1});

  TensorType error = TensorType::FromString(
      "0, 1, 4, 5;"
      "2, 3, 6, 7");

  TensorType gt_1 = TensorType::FromString(
      "0, 1, 4, 5;"
      "-2, -3, -6, -7");

  TensorType gt_2 = TensorType::FromString(
      "52;"
      "116");
  gt_2.Reshape({2, 1});

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values and shape
  ASSERT_TRUE(prediction[0].shape() == data_1.shape());
  ASSERT_TRUE(prediction[1].shape() == data_2.shape());
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MultiplyTest, backward_test_NB_NB)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType gt_1 = TensorType::FromString(
      "8,	   7,  12,  10,  12,   9,   8,  4;"
      "-40, -35, -36, -30,	-28, -21, -16, -8");

  TensorType gt_2 = TensorType::FromString(
      "1,   2,	 6,	  8, 15,  18, 28,  32;"
      "5, -10, 18,	-24, 35, -42, 56, -64");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(MultiplyTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Multiply<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Multiply<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType gt = TensorType::FromString(
      "8, 14, 18,20, 20,18, 14,8;"
      "-8,  14,-18, 20,-20, 18,-14, 8");

  OpType op;

  TensorType    prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  VecTensorType vec_data(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)});

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
  TensorType new_prediction(op.ComputeOutputShape(
      {std::make_shared<const TensorType>(data_1), std::make_shared<const TensorType>(data_2)}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, DataType{0}, DataType{0}));
}

TYPED_TEST(MultiplyTest, saveparams_backward_test_NB_NB)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::Multiply<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8, -7, 6,-5, 4,-3, 2,-1;"
      "-8,  7,-6, 5,-4, 3,-2, 1");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, -4;"
      "5, -5, 6, -6, 7, -7, 8, -8");

  fetch::ml::ops::Multiply<TensorType> op;
  std::vector<TensorType>              prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // extract saveparams
  std::shared_ptr<fetch::ml::OpsSaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::MsgPackSerializer b;
  b << *dsp;

  // make another prediction with the original op
  prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  std::vector<TensorType> new_prediction = new_op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  EXPECT_TRUE(prediction.at(0).AllClose(
      new_prediction.at(0), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));

  ASSERT_TRUE(!math::state_overflow<typename TypeParam::Type>());
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
