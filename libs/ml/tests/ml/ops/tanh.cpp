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

#include "ml/ops/tanh.hpp"

#include "math/base_types.hpp"
#include "math/tensor.hpp"
#include "vectorise/fixed_point/fixed_point.hpp"
#include "vectorise/fixed_point/serializers.hpp"

#include "gtest/gtest.h"

#include <cstdint>
#include <vector>

template <typename T>
class TanhTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(TanhTest, MyTypes);

TYPED_TEST(TanhTest, forward_all_positive_test)
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

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-4), typename TypeParam::Type(1e-4)));
}

TYPED_TEST(TanhTest, forward_all_negative_test)
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

  TypeParam prediction(op.ComputeOutputShape({data}));
  op.Forward({data}, prediction);

  ASSERT_TRUE(
      prediction.AllClose(gt, typename TypeParam::Type(1e-4), typename TypeParam::Type(1e-4)));
}

TYPED_TEST(TanhTest, backward_all_positive_test)
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
  std::vector<TypeParam>          prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(
      prediction[0].AllClose(gt, typename TypeParam::Type(1e-4), typename TypeParam::Type(1e-4)));
}

TYPED_TEST(TanhTest, backward_all_negative_test)
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
  std::vector<TypeParam>          prediction = op.Backward({data}, error);

  // test correct values
  ASSERT_TRUE(
      prediction[0].AllClose(gt, typename TypeParam::Type(1e-4), typename TypeParam::Type(1e-4)));
}

TYPED_TEST(TanhTest, saveparams_test)
{
  using ArrayType     = TypeParam;
  using DataType      = typename TypeParam::Type;
  using VecTensorType = typename fetch::ml::Ops<ArrayType>::VecTensorType;
  using SPType        = typename fetch::ml::ops::TanH<ArrayType>::SPType;
  using OpType        = typename fetch::ml::ops::TanH<ArrayType>;

  ArrayType data = TypeParam::FromString("0, 0.2, 0.4, -0, -0.2, -0.4");
  ArrayType gt   = TypeParam::FromString("0.0, 0.197375, 0.379949, -0.0, -0.197375, -0.379949");

  OpType op;

  ArrayType     prediction(op.ComputeOutputShape({data}));
  VecTensorType vec_data({data});

  op.Forward(vec_data, prediction);

  // extract saveparams
  std::shared_ptr<fetch::ml::SaveableParams> sp = op.GetOpSaveableParams();

  // downcast to correct type
  auto dsp = std::dynamic_pointer_cast<SPType>(sp);

  // serialize
  fetch::serializers::ByteArrayBuffer b;
  b << *dsp;

  // deserialize
  b.seek(0);
  auto dsp2 = std::make_shared<SPType>();
  b >> *dsp2;

  // rebuild node
  OpType new_op(*dsp2);

  // check that new predictions match the old
  ArrayType new_prediction(op.ComputeOutputShape({data}));
  new_op.Forward(vec_data, new_prediction);

  // test correct values
  EXPECT_TRUE(new_prediction.AllClose(prediction, fetch::math::function_tolerance<DataType>(),
                                      fetch::math::function_tolerance<DataType>()));
}
