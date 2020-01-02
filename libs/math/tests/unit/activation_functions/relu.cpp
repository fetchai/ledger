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

#include "gtest/gtest.h"
#include "math/activation_functions/relu.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class ReluTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ReluTest, TensorIntAndFloatingTypes);

template <typename ArrayType>
ArrayType RandomArrayNegative(std::size_t n)
{
  using DataType = typename ArrayType::Type;
  ;
  ArrayType a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = -random::Random::generator.AsType<DataType>() - DataType{1};
  }
  return a1;
}

template <typename ArrayType>
ArrayType RandomArrayPositive(std::size_t n)
{
  using DataType = typename ArrayType::Type;
  ;
  ArrayType a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = random::Random::generator.AsType<DataType>();
  }
  return a1;
}

TYPED_TEST(ReluTest, negative_response)
{
  using DataType         = typename TypeParam::Type;
  std::size_t n          = 1000;
  TypeParam   test_array = RandomArrayNegative<TypeParam>(n);
  TypeParam   test_array_2{n};

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_LT(test_array[i], DataType{0});
  }

  //
  fetch::math::Relu(test_array, test_array_2);

  // check that all values 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_EQ(test_array_2[i], DataType{0});
  }
}

TYPED_TEST(ReluTest, positive_response)
{
  using DataType = typename TypeParam::Type;

  std::size_t n          = 1000;
  TypeParam   test_array = RandomArrayPositive<TypeParam>(n);
  TypeParam   test_array_2{n};

  // sanity check that all values gte 0
  for (std::size_t i = 0; i < n; ++i)
  {
    ASSERT_GE(test_array[i], DataType{0});
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

}  // namespace test
}  // namespace math
}  // namespace fetch
