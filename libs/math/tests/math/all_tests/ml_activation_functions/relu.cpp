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

#include "math/ml/activation_functions/relu.hpp"
#include "math/tensor.hpp"

#include "gtest/gtest.h"

template <typename T>
class ReluTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<int>, fetch::math::Tensor<float>,
                                 fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(ReluTest, MyTypes);

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

TYPED_TEST(ReluTest, negative_response)
{

  std::size_t n          = 1000;
  TypeParam   test_array = RandomArrayNegative<TypeParam>(n);
  TypeParam   test_array_2{n};

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_LT(test_array[i], typename TypeParam::Type(0));
  }

  //
  fetch::math::Relu(test_array, test_array_2);

  // check that all values 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_EQ(test_array_2[i], typename TypeParam::Type(0));
  }
}

TYPED_TEST(ReluTest, positive_response)
{

  std::size_t n          = 1000;
  TypeParam   test_array = RandomArrayPositive<TypeParam>(n);
  TypeParam   test_array_2{n};

  // sanity check that all values gte 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_GE(test_array[i], typename TypeParam::Type(0));
  }

  fetch::math::Relu(test_array, test_array_2);
  ASSERT_EQ(test_array.size(), test_array_2.size());
  ASSERT_EQ(test_array.shape(), test_array_2.shape());

  // check that all values unchanged
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_EQ(test_array_2[i], test_array[i]);
  }
}
