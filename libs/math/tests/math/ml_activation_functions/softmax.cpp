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

#include <iomanip>
#include <iostream>

#include "math/free_functions/ml/activation_functions/softmax.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class SoftmaxTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(SoftmaxTest, MyTypes);

template <typename ArrayType>
ArrayType RandomArrayNegative(std::size_t n)
{
  static fetch::random::LinearCongruentialGenerator gen;
  ArrayType                                         a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = typename ArrayType::Type(gen.AsDouble()) - typename ArrayType::Type(1.0);
  }
  return a1;
}

template <typename ArrayType>
ArrayType RandomArrayPositive(std::size_t n)
{
  static fetch::random::LinearCongruentialGenerator gen;
  ArrayType                                         a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = typename ArrayType::Type(gen.AsDouble());
  }
  return a1;
}

TYPED_TEST(SoftmaxTest, equal_proportion_test)
{

  std::size_t n = 1000;
  TypeParam   test_array{n};
  TypeParam   result_array{n};
  for (auto &e : test_array)
  {
    e = typename TypeParam::Type(1);
  }

  //
  fetch::math::Softmax(test_array, result_array);

  // check that all values equal proportion
  ASSERT_EQ(result_array[0], typename TypeParam::Type(1.0 / double(n)));
  for (std::size_t i = 1; i < n; ++i)
  {
    ASSERT_EQ(result_array[i], result_array[0]);
  }
}

TYPED_TEST(SoftmaxTest, exact_values_test)
{

  TypeParam test_array{8};
  TypeParam gt_array{8};

  test_array[0] = typename TypeParam::Type(1);
  test_array[1] = typename TypeParam::Type(-2);
  test_array[2] = typename TypeParam::Type(3);
  test_array[3] = typename TypeParam::Type(-4);
  test_array[4] = typename TypeParam::Type(5);
  test_array[5] = typename TypeParam::Type(-6);
  test_array[6] = typename TypeParam::Type(7);
  test_array[7] = typename TypeParam::Type(-8);

  gt_array[0] = typename TypeParam::Type(2.1437e-03);
  gt_array[1] = typename TypeParam::Type(1.0673e-04);
  gt_array[2] = typename TypeParam::Type(1.5840e-02);
  gt_array[3] = typename TypeParam::Type(1.4444e-05);
  gt_array[4] = typename TypeParam::Type(1.1704e-01);
  gt_array[5] = typename TypeParam::Type(1.9548e-06);
  gt_array[6] = typename TypeParam::Type(8.6485e-01);
  gt_array[7] = typename TypeParam::Type(2.6456e-07);

  fetch::math::Softmax(test_array, test_array);

  // test correct values
  ASSERT_TRUE(test_array.AllClose(gt_array, typename TypeParam::Type(1e-5),
                                  typename TypeParam::Type(1e-5)));
}