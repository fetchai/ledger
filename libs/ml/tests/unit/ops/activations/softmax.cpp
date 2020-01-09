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
#include "ml/ops/activations/softmax.hpp"
#include "ml/serializers/ml_types.hpp"
#include "test_types.hpp"
#include <memory>

namespace fetch {
namespace ml {
namespace test {
template <typename T>
class SoftmaxTest : public ::testing::Test
{
};

TYPED_TEST_CASE(SoftmaxTest, math::test::HighPrecisionTensorFloatingTypes);

TYPED_TEST(SoftmaxTest, forward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data = TensorType::FromString("1; -2; 3; -4; 5; -6; 7; -8");
  TensorType gt   = TensorType::FromString(
      "0.0021437; 0.00010673; 0.01584; 0.000014444; 0.11704; 0.0000019548; 0.86485; "
      "0.00000026456");

  fetch::ml::ops::Softmax<TensorType> op(0);
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::Type<DataType>("0.00001"),
                                  fetch::math::Type<DataType>("0.00001")));
}

TYPED_TEST(SoftmaxTest, forward_2d_tensor_axis_1_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({3, 3, 1});
  TensorType          gt({3, 3, 1});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9});
  std::vector<double> gt_input({1.1850e-01, 5.8998e-03, 8.7560e-01, 1.2339e-04, 9.9986e-01,
                                1.6699e-05, 1.1920e-01, 3.6464e-08, 8.8080e-01});
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      data.Set(i, j, 0, fetch::math::AsType<DataType>(data_input[j + 3 * i]));
      gt.Set(i, j, 0, fetch::math::AsType<DataType>(gt_input[j + 3 * i]));
    }
  }

  fetch::ml::ops::Softmax<TensorType> op{1};
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::Type<DataType>("0.0001"),
                                  fetch::math::Type<DataType>("0.0001")));
}

TYPED_TEST(SoftmaxTest, backward_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;

  TensorType data  = TensorType::FromString("1; -2; 3; -4; 5; -6; 7; -8");
  TensorType error = TensorType::FromString("0; 0; 0; 0; 1; 0; 0; 0");
  TensorType gt    = TensorType::FromString(
      "-0.00025091; -0.000012492; -0.001854; -0.0000016906; 0.10335; -0.0000002288; -0.10123; "
      "-0.000000030965");

  fetch::ml::ops::Softmax<TensorType> op(0);
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::Type<DataType>("0.00001"),
                                     fetch::math::Type<DataType>("0.00001")));
}

TYPED_TEST(SoftmaxTest, backward_2d_tensor_axis_1_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({3, 3, 1});
  TensorType          error({3, 3, 1});
  TensorType          gt({3, 3, 1});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8, 9});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0, 0});
  std::vector<double> gt_input({0, 0, 0, -1.2338e-04, 1.4005e-04, -1.6697e-05, 0, 0, 0});
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      data.Set(i, j, 0, fetch::math::AsType<DataType>(data_input[j + 3 * i]));
      error.Set(i, j, 0, fetch::math::AsType<DataType>(errorInput[j + 3 * i]));
      gt.Set(i, j, 0, fetch::math::AsType<DataType>(gt_input[j + 3 * i]));
    }
  }

  fetch::ml::ops::Softmax<TensorType> op{1};
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SoftmaxTest, forward_3d_tensor_axis_1_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 2, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input(
      {0.119203, 0.880797, 0.880797, 0.119203, 0.119203, 0.880797, 0.880797, 0.119203});

  SizeType cnt  = 0;
  auto     it_d = data.begin();
  auto     it_g = gt.begin();

  while (it_d.is_valid())
  {
    *it_d = fetch::math::AsType<DataType>(data_input.at(cnt));
    *it_g = fetch::math::AsType<DataType>(gt_input.at(cnt));

    cnt++;
    ++it_d;
    ++it_g;
  }

  fetch::ml::ops::Softmax<TensorType> op{1};
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::Type<DataType>("0.0001"),
                                  fetch::math::Type<DataType>("0.0001")));
}

TYPED_TEST(SoftmaxTest, backward_3d_tensor_axis_1_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 2, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0});
  std::vector<double> gt_input({0, 0, 0, 0, 0.104994, 0, -0.104994, 0});

  SizeType cnt  = 0;
  auto     it_d = data.begin();
  auto     it_e = error.begin();
  auto     it_g = gt.begin();

  while (it_d.is_valid())
  {
    *it_d = fetch::math::AsType<DataType>(data_input.at(cnt));
    *it_e = fetch::math::AsType<DataType>(errorInput.at(cnt));
    *it_g = fetch::math::AsType<DataType>(gt_input.at(cnt));

    cnt++;
    ++it_d;
    ++it_e;
    ++it_g;
  }

  fetch::ml::ops::Softmax<TensorType> op{1};
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::Type<DataType>("0.00001"),
                                     fetch::math::Type<DataType>("0.00001")));
}

TYPED_TEST(SoftmaxTest, forward_3d_tensor_axis_0_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 2, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input(
      {0.952574, 0.0474259, 0.999089, 0.000911051, 0.999983, 1.67014e-05, 1, 3.05902e-07});

  SizeType cnt  = 0;
  auto     it_d = data.begin();
  auto     it_g = gt.begin();

  while (it_d.is_valid())
  {
    *it_d = fetch::math::AsType<DataType>(data_input.at(cnt));
    *it_g = fetch::math::AsType<DataType>(gt_input.at(cnt));

    cnt++;
    ++it_d;
    ++it_g;
  }

  fetch::ml::ops::Softmax<TensorType> op{0};
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::Type<DataType>("0.0001"),
                                  fetch::math::Type<DataType>("0.0001")));
}

TYPED_TEST(SoftmaxTest, backward_3d_tensor_axis_0_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 2, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0});
  std::vector<double> gt_input({0, 0, 0, 0, 1.67011e-05, -1.67011e-05, 0, 0});

  SizeType cnt  = 0;
  auto     it_d = data.begin();
  auto     it_e = error.begin();
  auto     it_g = gt.begin();

  while (it_d.is_valid())
  {
    *it_d = fetch::math::AsType<DataType>(data_input.at(cnt));
    *it_e = fetch::math::AsType<DataType>(errorInput.at(cnt));
    *it_g = fetch::math::AsType<DataType>(gt_input.at(cnt));

    cnt++;
    ++it_d;
    ++it_e;
    ++it_g;
  }

  fetch::ml::ops::Softmax<TensorType> op{0};
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SoftmaxTest, forward_3d_tensor_axes_0_2_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 2, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> gt_input({0.0179698, 0.000894665, 0.0179859, 1.6401e-05, 0.981119,
                                1.63864e-05, 0.981997, 3.00395e-07});

  SizeType cnt  = 0;
  auto     it_d = data.begin();
  auto     it_g = gt.begin();

  while (it_d.is_valid())
  {
    *it_d = fetch::math::AsType<DataType>(data_input.at(cnt));
    *it_g = fetch::math::AsType<DataType>(gt_input.at(cnt));

    cnt++;
    ++it_d;
    ++it_g;
  }

  fetch::ml::ops::Softmax<TensorType> op({0, 2});
  TensorType prediction(op.ComputeOutputShape({std::make_shared<const TensorType>(data)}));
  op.Forward({std::make_shared<const TensorType>(data)}, prediction);

  // test correct values
  ASSERT_TRUE(prediction.AllClose(gt, fetch::math::Type<DataType>("0.0001"),
                                  fetch::math::Type<DataType>("0.0001")));
}

TYPED_TEST(SoftmaxTest, backward_3d_tensor_axes_0_2_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;

  TensorType          data({2, 2, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0});
  std::vector<double> gt_input({-0.0176305, -0.000877773, 0, 0, 0.0185244, -1.6077e-05, 0, 0});

  SizeType cnt  = 0;
  auto     it_d = data.begin();
  auto     it_e = error.begin();
  auto     it_g = gt.begin();

  while (it_d.is_valid())
  {
    *it_d = fetch::math::AsType<DataType>(data_input.at(cnt));
    *it_e = fetch::math::AsType<DataType>(errorInput.at(cnt));
    *it_g = fetch::math::AsType<DataType>(gt_input.at(cnt));

    cnt++;
    ++it_d;
    ++it_e;
    ++it_g;
  }

  fetch::ml::ops::Softmax<TensorType> op({0, 2});
  std::vector<TensorType>             prediction =
      op.Backward({std::make_shared<const TensorType>(data)}, error);

  // test correct values
  ASSERT_TRUE(prediction[0].AllClose(gt, fetch::math::function_tolerance<DataType>(),
                                     fetch::math::function_tolerance<DataType>()));
}

TYPED_TEST(SoftmaxTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::Softmax<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::Softmax<TensorType>;

  TensorType data = TensorType::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TensorType gt   = TensorType::FromString(
      "0.0021437, 0.00010673, 0.01584, 0.000014444, 0.11704, 0.0000019548, 0.86485, "
      "0.00000026456");

  fetch::ml::ops::Softmax<TensorType> op(0);
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

TYPED_TEST(SoftmaxTest, saveparams_backward_3d_tensor_axes_0_2_test)
{
  using DataType   = typename TypeParam::Type;
  using TensorType = TypeParam;
  using SizeType   = fetch::math::SizeType;
  using OpType     = typename fetch::ml::ops::Softmax<TensorType>;
  using SPType     = typename OpType::SPType;

  TensorType          data({2, 2, 2});
  TensorType          error({2, 2, 2});
  TensorType          gt({2, 2, 2});
  std::vector<double> data_input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<double> errorInput({0, 0, 0, 0, 1, 0, 0, 0});
  std::vector<double> gt_input({-0.0176305, -0.000877773, 0, 0, 0.0185244, -1.6077e-05, 0, 0});

  SizeType cnt  = 0;
  auto     it_d = data.begin();
  auto     it_e = error.begin();
  auto     it_g = gt.begin();

  while (it_d.is_valid())
  {
    *it_d = fetch::math::AsType<DataType>(data_input.at(cnt));
    *it_e = fetch::math::AsType<DataType>(errorInput.at(cnt));
    *it_g = fetch::math::AsType<DataType>(gt_input.at(cnt));

    cnt++;
    ++it_d;
    ++it_e;
    ++it_g;
  }

  fetch::ml::ops::Softmax<TensorType> op({0, 2});

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
