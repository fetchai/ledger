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

#include "core/serializers/main_serializer_definition.hpp"
#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "ml/ops/tanh.hpp"
#include "ml/serializers/ml_types.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"
#include <cstdint>
#include <vector>

template <typename T>
class TanHTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(TanHTest, MyTypes);

TYPED_TEST(TanHTest, forward_all_positive_test)
{
  uint8_t   n = 9;
  TypeParam data{n};
  TypeParam gt({n});

  std::vector<double> dataInput({0, 0.2, 0.4, 0.6, 0.8, 1, 1.2, 1.4, 10});
  std::vector<double> gtInput(
      {0.0, 0.197375, 0.379949, 0.53705, 0.664037, 0.761594, 0.833655, 0.885352, 1.0});

  for (std::uint64_t i(0); i < n; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    gt.Set(i, typename TypeParam::Type(gtInput[i]));
  }

  fetch::ml::ops::TanH<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TypeParam>(data)}));
  op.Forward({std::make_shared<const TypeParam>(data)}, prediction);

  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-4), typename TypeParam::Type(1e-4)));
}

TYPED_TEST(TanHTest, forward_all_negative_test)
{
  uint8_t   n = 9;
  TypeParam data{n};
  TypeParam gt{n};

  std::vector<double> dataInput({-0, -0.2, -0.4, -0.6, -0.8, -1, -1.2, -1.4, -10});
  std::vector<double> gtInput(
      {-0.0, -0.197375, -0.379949, -0.53705, -0.664037, -0.761594, -0.833655, -0.885352, -1.0});

  for (std::uint64_t i(0); i < n; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    gt.Set(i, typename TypeParam::Type(gtInput[i]));
  }

  fetch::ml::ops::TanH<TypeParam> op;

  TypeParam prediction(op.ComputeOutputShape({std::make_shared<const TypeParam>(data)}));
  op.Forward({std::make_shared<const TypeParam>(data)}, prediction);

  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-4), typename TypeParam::Type(1e-4)));
}

TYPED_TEST(TanHTest, backward_all_positive_test)
{

  uint8_t             n = 8;
  TypeParam           data{n};
  TypeParam           error{n};
  TypeParam           gt{n};
  std::vector<double> dataInput({0, 0.2, 0.4, 0.6, 0.8, 1.2, 1.4, 10});
  std::vector<double> errorInput({{0.2, 0.1, 0.3, 0.2, 0.5, 0.1, 0.0, 0.3}});
  std::vector<double> gtInput({0.2, 0.096104, 0.256692, 0.142316, 0.279528, 0.030502, 0.0, 0.0});
  for (std::uint64_t i(0); i < n; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    error.Set(i, typename TypeParam::Type(errorInput[i]));
    gt.Set(i, typename TypeParam::Type(gtInput[i]));
  }
  fetch::ml::ops::TanH<TypeParam> op;
  std::vector<TypeParam> prediction = op.Backward({std::make_shared<const TypeParam>(data)}, error);

  // test correct values
  ASSERT_TRUE(
      prediction[0].AllClose(gt, typename TypeParam::Type(1e-4), typename TypeParam::Type(1e-4)));
}

TYPED_TEST(TanHTest, backward_all_negative_test)
{
  uint8_t             n = 8;
  TypeParam           data{n};
  TypeParam           error{n};
  TypeParam           gt{n};
  std::vector<double> dataInput({-0, -0.2, -0.4, -0.6, -0.8, -1.2, -1.4, -10});
  std::vector<double> errorInput({{-0.2, -0.1, -0.3, -0.2, -0.5, -0.1, -0.0, -0.3}});
  std::vector<double> gtInput(
      {-0.2, -0.096104, -0.256692, -0.142316, -0.279528, -0.030502, 0.0, 0.0});
  for (std::uint64_t i(0); i < n; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    error.Set(i, typename TypeParam::Type(errorInput[i]));
    gt.Set(i, typename TypeParam::Type(gtInput[i]));
  }
  fetch::ml::ops::TanH<TypeParam> op;
  std::vector<TypeParam> prediction = op.Backward({std::make_shared<const TypeParam>(data)}, error);

  // test correct values
  ASSERT_TRUE(
      prediction[0].AllClose(gt, typename TypeParam::Type(1e-4), typename TypeParam::Type(1e-4)));
}

TYPED_TEST(TanHTest, saveparams_test)
{
  using TensorType    = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::ops::Ops<TensorType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::TanH<TensorType>::SPType;
  using OpType        = typename fetch::ml::ops::TanH<TensorType>;

  TensorType data = TypeParam::FromString("0, 0.2, 0.4, -0, -0.2, -0.4");
  TensorType gt   = TypeParam::FromString("0.0, 0.197375, 0.379949, -0.0, -0.197375, -0.379949");

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
  EXPECT_TRUE(
      new_prediction.AllClose(prediction, static_cast<DataType>(0), static_cast<DataType>(0)));
}

TYPED_TEST(TanHTest, saveparams_backward_all_negative_test)
{
  using TensorType = TypeParam;
  using OpType     = typename fetch::ml::ops::TanH<TensorType>;
  using SPType     = typename OpType ::SPType;

  uint8_t             n = 8;
  TypeParam           data{n};
  TypeParam           error{n};
  std::vector<double> dataInput({-0, -0.2, -0.4, -0.6, -0.8, -1.2, -1.4, -10});
  std::vector<double> errorInput({{-0.2, -0.1, -0.3, -0.2, -0.5, -0.1, -0.0, -0.3}});

  for (std::uint64_t i(0); i < n; ++i)
  {
    data.Set(i, typename TypeParam::Type(dataInput[i]));
    error.Set(i, typename TypeParam::Type(errorInput[i]));
  }

  fetch::ml::ops::TanH<TypeParam> op;
  std::vector<TypeParam> prediction = op.Backward({std::make_shared<const TypeParam>(data)}, error);

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
