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
#include "ml/ops/add.hpp"
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
class AddTest : public ::testing::Test
{
};

TYPED_TEST_CASE(AddTest, math::test::TensorFloatingTypes);

TYPED_TEST(AddTest, forward_test_NB_N1)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8;"
      "-8");

  TensorType gt = TensorType::FromString(
      "9,  6, 11,  4, 13,  2, 15, 0;"
      "-7, -6, -5, -4, -3,	-2,	-1,	0");

  fetch::ml::ops::Add<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}));
  op.Forward({std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)},
             prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(AddTest, forward_test_NB_NB)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "-1, 2, -3,4, -5,6, -7,8;"
      "-1, -2, -3, -4, -5, -6, -7, -8");

  TensorType gt(data_1.shape());

  fetch::ml::ops::Add<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}));
  op.Forward({std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)},
             prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(AddTest, backward_test_NMB_N11)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, -1, 1, 1;"
      "0, 1, 6, 2");
  data_1.Reshape({2, 2, 2});

  TensorType data_2 = TensorType::FromString("1, -1");
  data_2.Reshape({2, 1, 1});

  TensorType gt = TensorType::FromString(
      "14;"
      "22");

  TensorType error = TensorType::FromString(
      "1, 2, 5, 6;"
      "3, 4, 7, 8");
  error.Reshape({2, 2, 2});

  fetch::ml::ops::Add<TensorType> op;
  std::vector<TensorType>         prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values and shape
  ASSERT_TRUE(prediction[1].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].shape() == data_2.shape());
}

TYPED_TEST(AddTest, backward_test_NMB_111)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, -1, 1, 1;"
      "0, 1, 6, 2");
  data_1.Reshape({2, 2, 2});

  TensorType data_2 = TensorType::FromString("1");
  data_2.Reshape({1, 1, 1});

  TensorType gt = TensorType::FromString("36");

  TensorType error = TensorType::FromString(
      "1, 2, 5, 6;"
      "3, 4, 7, 8");
  error.Reshape({2, 2, 2});

  fetch::ml::ops::Add<TensorType> op;
  std::vector<TensorType>         prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  ASSERT_TRUE(prediction[1].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].shape() == data_2.shape());
}

TYPED_TEST(AddTest, backward_test_NB_N1)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8;"
      "-8");

  TensorType gt_1 = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, 4;"
      "5, -5, 6, -6, 7, -7, 8, 8");

  TensorType gt_2 = TensorType::FromString(
      "8;"
      "16");

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, 4;"
      "5, -5, 6, -6, 7, -7, 8, 8");

  fetch::ml::ops::Add<TensorType> op;
  std::vector<TensorType>         prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  ASSERT_TRUE(prediction[1].shape() == data_2.shape());
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(AddTest, forward_2D_broadcast_test)
{
  using TensorType = TypeParam;
  using DataType   = typename TypeParam::Type;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType({1, 1});
  data_2.At(0, 0)   = static_cast<DataType>(8);

  TensorType gt = TensorType::FromString(
      "9,  6, 11,  4, 13,  2, 15, 0;"
      "9, 10, 11, 12, 13,	14,	15,	16");

  fetch::ml::ops::Add<TensorType> op;

  TypeParam prediction(op.ComputeOutputShape(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}));
  op.Forward({std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)},
             prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                  fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(AddTest, backward_2D_broadcast_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType({1, 1});
  data_2.At(0, 0)   = static_cast<DataType>(8);

  TensorType gt_1 = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, 4;"
      "5, -5, 6, -6, 7, -7, 8, 8");

  TensorType gt_2 = TensorType({1, 1});
  gt_2.At(0, 0)   = static_cast<DataType>(24);

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, 4;"
      "5, -5, 6, -6, 7, -7, 8, 8");

  fetch::ml::ops::Add<TensorType> op;
  std::vector<TensorType>         prediction = op.Backward(
      {std::make_shared<TensorType>(data_1), std::make_shared<TensorType>(data_2)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt_1, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
  ASSERT_TRUE(prediction[1].AllClose(gt_2, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(AddTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Add<TensorType>::SPType;
  using OpType        = fetch::ml::ops::Add<TensorType>;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType::FromString(
      "8;"
      "-8");

  TensorType gt = TensorType::FromString(
      "9,  6, 11,  4, 13,  2, 15, 0;"
      "-7, -6, -5, -4, -3,	-2,	-1,	0");

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

TYPED_TEST(AddTest, saveparams_backward_2D_broadcast_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using OpType     = fetch::ml::ops::Add<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType data_1 = TensorType::FromString(
      "1, -2, 3,-4, 5,-6, 7,-8;"
      "1,  2, 3, 4, 5, 6, 7, 8");

  TensorType data_2 = TensorType({1, 1});
  data_2.At(0, 0)   = static_cast<DataType>(8);

  TensorType error = TensorType::FromString(
      "1, -1, 2, -2, 3, -3, 4, 4;"
      "5, -5, 6, -6, 7, -7, 8, 8");

  fetch::ml::ops::Add<TensorType> op;
  std::vector<TensorType>         prediction = op.Backward(
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
  // test correct values
  EXPECT_TRUE(prediction.at(1).AllClose(
      new_prediction.at(1), fetch::math::function_tolerance<typename TypeParam::Type>(),
      fetch::math::function_tolerance<typename TypeParam::Type>()));
}

}  // namespace test
}  // namespace ml
}  // namespace fetch
