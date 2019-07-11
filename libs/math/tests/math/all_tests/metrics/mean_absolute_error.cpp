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

#include "math/metrics/mean_absolute_error.hpp"
#include "math/tensor.hpp"

#include "gtest/gtest.h"

template <typename T>
class MeanAbsoluteErrorTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
<<<<<<< HEAD
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
=======
                                 fetch::math::Tensor<fetch::fixed_point::fp32_t>,
                                 fetch::math::Tensor<fetch::fixed_point::fp64_t>>;
>>>>>>> a8952cdbe0db8c87f4de751fddd042fc25debe88
TYPED_TEST_CASE(MeanAbsoluteErrorTest, MyTypes);

TYPED_TEST(MeanAbsoluteErrorTest, perfect_match_test)
{
<<<<<<< HEAD
  TypeParam test_array = TypeParam{8};
  TypeParam gt_array   = TypeParam{8};

  test_array[0] = typename TypeParam::Type(1);
  test_array[1] = typename TypeParam::Type(-2);
  test_array[2] = typename TypeParam::Type(3);
  test_array[3] = typename TypeParam::Type(-4);
  test_array[4] = typename TypeParam::Type(5);
  test_array[5] = typename TypeParam::Type(-6);
  test_array[6] = typename TypeParam::Type(7);
  test_array[7] = typename TypeParam::Type(-8);

  gt_array[0] = typename TypeParam::Type(1);
  gt_array[1] = typename TypeParam::Type(-2);
  gt_array[2] = typename TypeParam::Type(3);
  gt_array[3] = typename TypeParam::Type(-4);
  gt_array[4] = typename TypeParam::Type(5);
  gt_array[5] = typename TypeParam::Type(-6);
  gt_array[6] = typename TypeParam::Type(7);
  gt_array[7] = typename TypeParam::Type(-8);
=======
  TypeParam test_array = TypeParam::FromString("1, -2, 3, -4, 5, -6, 7, -8");
  TypeParam gt_array   = TypeParam::FromString("1, -2, 3, -4, 5, -6, 7, -8");
>>>>>>> a8952cdbe0db8c87f4de751fddd042fc25debe88

  // initialise to non-zero just to avoid correct value at initialisation
  typename TypeParam::Type score(100);
  score = fetch::math::MeanAbsoluteError(test_array, gt_array);

  // test correct values
<<<<<<< HEAD
  ASSERT_NEAR(double(score), double(0.0), double(1.0e-5f));
=======
  ASSERT_NEAR(static_cast<double>(score), double(0.0),
              static_cast<double>(test_array.size()) *
                  static_cast<double>(fetch::math::function_tolerance<typename TypeParam::Type>()));
>>>>>>> a8952cdbe0db8c87f4de751fddd042fc25debe88
}

TYPED_TEST(MeanAbsoluteErrorTest, value_test)
{
<<<<<<< HEAD
  TypeParam test_array = TypeParam{8};
  TypeParam gt_array   = TypeParam{8};

  test_array[0] = typename TypeParam::Type(1.1);
  test_array[1] = typename TypeParam::Type(-2.2);
  test_array[2] = typename TypeParam::Type(3.3);
  test_array[3] = typename TypeParam::Type(-4.4);
  test_array[4] = typename TypeParam::Type(5.5);
  test_array[5] = typename TypeParam::Type(-6.6);
  test_array[6] = typename TypeParam::Type(7.7);
  test_array[7] = typename TypeParam::Type(-8.8);

  gt_array[0] = typename TypeParam::Type(1.1);
  gt_array[1] = typename TypeParam::Type(2.2);
  gt_array[2] = typename TypeParam::Type(7.7);
  gt_array[3] = typename TypeParam::Type(6.6);
  gt_array[4] = typename TypeParam::Type(0.0);
  gt_array[5] = typename TypeParam::Type(-6.6);
  gt_array[6] = typename TypeParam::Type(7.7);
  gt_array[7] = typename TypeParam::Type(-9.9);

  // initialise to non-zero just to avoid correct value at initialisation
=======
  TypeParam test_array = TypeParam::FromString("1.1, -2.2, 3.3, -4.4, 5.5, -6.6, 7.7, -8.8");
  TypeParam gt_array   = TypeParam::FromString("1.1, 2.2, 7.7, 6.6, 0.0, -6.6, 7.7, -9.9");

  // initialise to zero just to avoid correct value at initialisation
>>>>>>> a8952cdbe0db8c87f4de751fddd042fc25debe88
  typename TypeParam::Type score(0);
  score = fetch::math::MeanAbsoluteError(test_array, gt_array);

  // test correct values
<<<<<<< HEAD
  ASSERT_NEAR(double(score), double(26.4f / 8.0f), double(1.0e-5f));
=======
  ASSERT_NEAR(static_cast<double>(score), double(3.3f),
              static_cast<double>(test_array.size()) *
                  static_cast<double>(fetch::math::function_tolerance<typename TypeParam::Type>()));
>>>>>>> a8952cdbe0db8c87f4de751fddd042fc25debe88
}
