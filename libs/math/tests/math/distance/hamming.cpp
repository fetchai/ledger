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

#include "math/distance/hamming.hpp"
#include "math/tensor.hpp"
#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>

using namespace fetch::math::distance;
using namespace fetch::math;

template <typename T>
class HammingTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(HammingTest, MyTypes);

TYPED_TEST(HammingTest, simple_test)
{
  using DataType = typename TypeParam::Type;
  TypeParam A    = TypeParam(4);
  A.Set(0, DataType(1));
  A.Set(1, DataType(2));
  A.Set(2, DataType(3));
  A.Set(3, DataType(4));
  EXPECT_EQ(Hamming(A, A), 0);

  TypeParam B = TypeParam(4);
  B.Set(0, DataType(1));
  B.Set(1, DataType(2));
  B.Set(2, DataType(3));
  B.Set(3, DataType(2));

  EXPECT_EQ(Hamming(A, B), 1);

  TypeParam C = TypeParam(3);
  C.Set(0, DataType(1));
  C.Set(1, DataType(2));
  C.Set(2, DataType(3));

  TypeParam D = TypeParam(3);
  D.Set(0, DataType(1));
  D.Set(1, DataType(2));
  D.Set(2, DataType(9));

  EXPECT_EQ(Hamming(C, D), 1);
}
