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

#include "math/ml/activation_functions/sigmoid.hpp"
#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class SigmoidTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(SigmoidTest, MyTypes);

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

TYPED_TEST(SigmoidTest, negative_response)
{

  std::size_t n          = 1000;
  TypeParam   test_array = RandomArrayNegative<TypeParam>(n);
  TypeParam   test_array_2{n};

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] <= typename TypeParam::Type(0));
  }

  //
  fetch::math::Sigmoid(test_array, test_array_2);

  // check that all values 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array_2[i] < typename TypeParam::Type(0.5));
  }
}

TYPED_TEST(SigmoidTest, positive_response)
{

  std::size_t n          = 1000;
  TypeParam   test_array = RandomArrayPositive<TypeParam>(n);
  TypeParam   test_array_2{n};

  // sanity check that all values gte 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array[i] >= typename TypeParam::Type(0));
  }

  fetch::math::Sigmoid(test_array, test_array_2);
  ASSERT_TRUE(test_array.size() == test_array_2.size());
  ASSERT_TRUE(test_array.shape() == test_array_2.shape());

  // check that all values unchanged
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_TRUE(test_array_2[i] >= typename TypeParam::Type(0.5));
  }
}

TYPED_TEST(SigmoidTest, exact_values)
{
  std::size_t n = 8;
  TypeParam   test_array{n};
  TypeParam   gt_array{n};

  test_array[0] = typename TypeParam::Type(1);
  test_array[1] = typename TypeParam::Type(-2);
  test_array[2] = typename TypeParam::Type(3);
  test_array[3] = typename TypeParam::Type(-4);
  test_array[4] = typename TypeParam::Type(5);
  test_array[5] = typename TypeParam::Type(-6);
  test_array[6] = typename TypeParam::Type(7);
  test_array[7] = typename TypeParam::Type(-8);

  gt_array[0] = typename TypeParam::Type(0.73106);
  gt_array[1] = typename TypeParam::Type(0.1192029);
  gt_array[2] = typename TypeParam::Type(0.952574);
  gt_array[3] = typename TypeParam::Type(0.01798620996);
  gt_array[4] = typename TypeParam::Type(0.993307149);
  gt_array[5] = typename TypeParam::Type(0.002472623156635);
  gt_array[6] = typename TypeParam::Type(0.999088948806);
  gt_array[7] = typename TypeParam::Type(0.000335350130466);

  fetch::math::Sigmoid(test_array, test_array);
  ASSERT_TRUE(test_array.size() == gt_array.size());
  ASSERT_TRUE(test_array.shape() == gt_array.shape());

  // test correct values
  ASSERT_TRUE(test_array.AllClose(gt_array, typename TypeParam::Type(1e-5),
                                  typename TypeParam::Type(1e-5)));
}

///////////////////
/// Sigmoid 2x2 ///
///////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values

TYPED_TEST(SigmoidTest, sigmoid_2x2)
{
  TypeParam array1{{2, 2}};

  array1.Set(0, typename TypeParam::Type(0.3));
  array1.Set(1, typename TypeParam::Type(1.2));
  array1.Set(2, typename TypeParam::Type(0.7));
  array1.Set(3, typename TypeParam::Type(22));

  TypeParam output = fetch::math::Sigmoid(array1);

  TypeParam numpy_output{{2, 2}};

  numpy_output.Set(0, typename TypeParam::Type(0.57444252));
  numpy_output.Set(1, typename TypeParam::Type(0.76852478));
  numpy_output.Set(2, typename TypeParam::Type(0.66818777));
  numpy_output.Set(3, typename TypeParam::Type(1));

  ASSERT_TRUE(output.AllClose(numpy_output));
}

///////////////////
/// Sigmoid 1x1 ///
///////////////////
// Test sigmoid function output against numpy output for 2x2 input matrix of random values
TYPED_TEST(SigmoidTest, sigmoid_11)
{
  TypeParam input{1};
  TypeParam output{1};
  TypeParam numpy_output{1};

  input.Set(0, typename TypeParam::Type(0.3));
  numpy_output.Set(0, typename TypeParam::Type(0));

  output = fetch::math::Sigmoid(input);

  numpy_output[0] = typename TypeParam::Type(0.574442516811659);

  ASSERT_TRUE(output.AllClose(numpy_output));
}