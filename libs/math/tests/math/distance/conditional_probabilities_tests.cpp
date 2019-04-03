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

#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

#include "math/distance/conditional_probabilities.hpp"
#include "math/tensor.hpp"

using namespace fetch::math::distance;
using namespace fetch::math;

template <typename T>
class DistanceTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(DistanceTest, MyTypes);

TYPED_TEST(DistanceTest, conditional_distance)
{
  using DataType = typename TypeParam::Type;
  TypeParam A    = TypeParam({4, 4});

  A.Set({0, 0}, DataType(0.1));
  A.Set({0, 1}, DataType(0.2));
  A.Set({0, 2}, DataType(0.3));
  A.Set({0, 3}, DataType(0.4));

  A.Set({1, 0}, DataType(-0.1));
  A.Set({1, 1}, DataType(-0.2));
  A.Set({1, 2}, DataType(-0.3));
  A.Set({1, 3}, DataType(-0.4));

  A.Set({2, 0}, DataType(-1.1));
  A.Set({2, 1}, DataType(-1.2));
  A.Set({2, 2}, DataType(-1.3));
  A.Set({2, 3}, DataType(-1.4));

  A.Set({3, 0}, DataType(1.1));
  A.Set({3, 1}, DataType(1.2));
  A.Set({3, 2}, DataType(1.3));
  A.Set({3, 3}, DataType(1.4));

  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 2, 1, DataType(1.0)), 0.93083999, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 3, 0, DataType(1.5)), 0.75535699, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 3, 1, DataType(2.0)), 0.3277747, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 1, 2, DataType(1.0)), 0.19495178, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 0, 3, DataType(1.5)), 0.31466864, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 1, 3, DataType(2.0)), 0.17749937, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 2, 1, DataType(-1.0)), 0.93083999, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 3, 0, DataType(-1.5)), 0.75535699, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 3, 1, DataType(-2.0)), 0.3277747, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 1, 2, DataType(-1.0)), 0.19495178, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 0, 3, DataType(-1.5)), 0.31466864, 1e-4);
  EXPECT_NEAR((double)ConditionalProbabilitiesDistance(A, 1, 3, DataType(-2.0)), 0.17749937, 1e-4);
}