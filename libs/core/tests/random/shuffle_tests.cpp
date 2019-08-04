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

#include "core/random.hpp"
#include "gtest/gtest.h"
#include "vectorise/fixed_point/fixed_point.hpp"

template <typename T>
class ShuffleTest : public ::testing::Test
{
};

using MyTypes =
    ::testing::Types<int, float, double, fetch::fixed_point::fp32_t, fetch::fixed_point::fp64_t>;

TYPED_TEST_CASE(ShuffleTest, MyTypes);

TYPED_TEST(ShuffleTest, lfg_test)
{
  // fill input vector with ascending values
  std::size_t            vec_length = 10;
  std::vector<TypeParam> input_vector(vec_length);
  for (std::size_t j = 0; j < vec_length; ++j)
  {
    input_vector.at(j) = static_cast<TypeParam>(j);
  }
  std::vector<TypeParam> output_vector(vec_length);
  std::vector<TypeParam> gt{static_cast<TypeParam>(9), static_cast<TypeParam>(7),
                            static_cast<TypeParam>(6), static_cast<TypeParam>(5),
                            static_cast<TypeParam>(2), static_cast<TypeParam>(8),
                            static_cast<TypeParam>(0), static_cast<TypeParam>(3),
                            static_cast<TypeParam>(4), static_cast<TypeParam>(1)};

  fetch::random::LaggedFibonacciGenerator<> lfg(123456789);
  fetch::random::Shuffle(lfg, input_vector, output_vector);

  EXPECT_EQ(output_vector, gt);
}

TYPED_TEST(ShuffleTest, lcg_test)
{
  // fill input vector with ascending values
  std::size_t            vec_length = 10;
  std::vector<TypeParam> input_vector(vec_length);
  for (std::size_t j = 0; j < vec_length; ++j)
  {
    input_vector.at(j) = static_cast<TypeParam>(j);
  }
  std::vector<TypeParam> output_vector(vec_length);
  std::vector<TypeParam> gt{static_cast<TypeParam>(7), static_cast<TypeParam>(9),
                            static_cast<TypeParam>(3), static_cast<TypeParam>(2),
                            static_cast<TypeParam>(0), static_cast<TypeParam>(8),
                            static_cast<TypeParam>(1), static_cast<TypeParam>(4),
                            static_cast<TypeParam>(5), static_cast<TypeParam>(6)};

  fetch::random::LinearCongruentialGenerator lcg(123456789);
  fetch::random::Shuffle(lcg, input_vector, output_vector);

  EXPECT_EQ(output_vector, gt);
}
