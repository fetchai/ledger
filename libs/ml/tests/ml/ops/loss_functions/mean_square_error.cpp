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

#include "ml/ops/loss_functions/mean_square_error.hpp"
#include "math/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class MeanSquareErrorTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(MeanSquareErrorTest, MyTypes);

TYPED_TEST(MeanSquareErrorTest, perfect_match_forward_test)
{
  TypeParam     data1(8);
  TypeParam     data2(8);
  std::uint64_t i(0);
  for (int e : {1, -2, 3, -4, 5, -6, 7, -8})
  {
    data1.Set(i, typename TypeParam::Type(e));
    data2.Set(i, typename TypeParam::Type(e));
    i++;
  }

  fetch::ml::ops::MeanSquareError<TypeParam> op;
  EXPECT_EQ(op.Forward({data1, data2}), typename TypeParam::Type(0));
}

TYPED_TEST(MeanSquareErrorTest, one_dimensional_forward_test)
{
  TypeParam     data1(8);
  TypeParam     data2(8);
  std::uint64_t i(0);
  for (double e : {1.1, -2.2, 3.3, -4.4, 5.5, -6.6, 7.7, -8.8})
  {
    data1.Set(i, typename TypeParam::Type(e));
    i++;
  }
  i = 0;
  for (double e : {1.1, 2.2, 7.7, 6.6, 0.0, -6.6, 7.7, -9.9})
  {
    data2.Set(i, typename TypeParam::Type(e));
    i++;
  }

  fetch::ml::ops::MeanSquareError<TypeParam> op;
  ASSERT_FLOAT_EQ(float(op.Forward({data1, data2})), 191.18f / 8.0f / 2.0f);
  // fetch::math::MeanSquareError divided sum by number of element (ie 8 in this case)
  // and then further didivde by do (cf issue 343)
}

TYPED_TEST(MeanSquareErrorTest, one_dimensional_backward_test)
{
  TypeParam     data1(8);
  TypeParam     data2(8);
  TypeParam     gt(8);
  std::uint64_t i(0);
  for (float e : {1.1f, -2.2f, 3.3f, -4.4f, 5.5f, -6.6f, 7.7f, -8.8f})
  {
    data1.Set(i, typename TypeParam::Type(e));
    i++;
  }
  i = 0;
  for (float e : {1.1f, 2.2f, 7.7f, 6.6f, 0.0f, -6.6f, 7.7f, -9.9f})
  {
    data2.Set(i, typename TypeParam::Type(e));
    i++;
  }
  i = 0;
  for (float e : {0.0f, -4.4f, -4.4f, -11.0f, 5.5f, 0.0f, 0.0f, 1.1f})
  {
    gt.Set(i, typename TypeParam::Type(e));
    i++;
  }

  fetch::ml::ops::MeanSquareError<TypeParam> op;
  EXPECT_TRUE(op.Backward({data1, data2})
                  .AllClose(gt, typename TypeParam::Type(1e-5), typename TypeParam::Type(1e-5)));
}
