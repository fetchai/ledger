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

#include "math/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"

#include "gtest/gtest.h"

#include <cstdint>

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
  uint8_t   n = 10;
  TypeParam data{n};
  TypeParam gt({n});

  std::vector<double> dataInput({0, 0.2, 0.4, 0.6, 0.8, 1, 1.2, 1.4, 20, 100});
  std::vector<double> gtInput(
      {0.0, 0.197375, 0.379949, 0.53705, 0.664037, 0.761594, 0.833655, 0.885352, 1.0, 1.0});

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
  uint8_t   n = 10;
  TypeParam data{n};
  TypeParam gt{n};

  std::vector<double> dataInput({-0, -0.2, -0.4, -0.6, -0.8, -1, -1.2, -1.4, -20, -100});
  std::vector<double> gtInput({-0.0, -0.197375, -0.379949, -0.53705, -0.664037, -0.761594,
                               -0.833655, -0.885352, -1.0, -1.0});

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

  uint8_t             n = 9;
  TypeParam           data{n};
  TypeParam           error{n};
  TypeParam           gt{n};
  std::vector<double> dataInput({0, 0.2, 0.4, 0.6, 0.8, 1.2, 1.4, 20, 100});
  std::vector<double> errorInput({{0.2, 0.1, 0.3, 0.2, 0.5, 0.1, 0.0, 0.3, 0.4}});
  std::vector<double> gtInput(
      {0.2, 0.096104, 0.256692, 0.142316, 0.279528, 0.030502, 0.0, 0.0, 0.0});
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
  uint8_t             n = 9;
  TypeParam           data{n};
  TypeParam           error{n};
  TypeParam           gt{n};
  std::vector<double> dataInput({-0, -0.2, -0.4, -0.6, -0.8, -1.2, -1.4, -20, -100});
  std::vector<double> errorInput({{-0.2, -0.1, -0.3, -0.2, -0.5, -0.1, -0.0, -0.3, -0.4}});
  std::vector<double> gtInput(
      {-0.2, -0.096104, -0.256692, -0.142316, -0.279528, -0.030502, 0.0, 0.0, 0.0});
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
