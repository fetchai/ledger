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

#include "math/standard_functions/clamp.hpp"
#include "math/tensor.hpp"

using namespace fetch::math;

template <typename T>
class ClampTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(ClampTest, MyTypes);

TYPED_TEST(ClampTest, clamp_array_1D_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType A = ArrayType({6});

  A(0) = DataType(-10);
  A(1) = DataType(0);
  A(2) = DataType(1);
  A(3) = DataType(2);
  A(4) = DataType(3);
  A(5) = DataType(10);

  // Expected results
  ArrayType A_clamp_expected = ArrayType({6});
  A_clamp_expected(0)        = DataType(2);
  A_clamp_expected(1)        = DataType(2);
  A_clamp_expected(2)        = DataType(2);
  A_clamp_expected(3)        = DataType(2);
  A_clamp_expected(4)        = DataType(3);
  A_clamp_expected(5)        = DataType(3);

  // Compare results with expected results
  Clamp(DataType(2), DataType(3), A);

  ASSERT_TRUE(A.AllClose(A_clamp_expected, DataType{1e-5f}, DataType{1e-5f}));
}

TYPED_TEST(ClampTest, clamp_array_2D_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType A = ArrayType({2, 3});

  A(0, 0) = DataType(-10);
  A(0, 1) = DataType(0);
  A(0, 2) = DataType(1);
  A(1, 0) = DataType(2);
  A(1, 1) = DataType(3);
  A(1, 2) = DataType(10);

  // Expected results
  ArrayType A_clamp_expected = ArrayType({2, 3});
  A_clamp_expected(0, 0)     = DataType(2);
  A_clamp_expected(0, 1)     = DataType(2);
  A_clamp_expected(0, 2)     = DataType(2);
  A_clamp_expected(1, 0)     = DataType(2);
  A_clamp_expected(1, 1)     = DataType(3);
  A_clamp_expected(1, 2)     = DataType(3);

  // Compare results with expected results
  Clamp(DataType(2), DataType(3), A);

  ASSERT_TRUE(A.AllClose(A_clamp_expected, DataType{1e-5f}, DataType{1e-5f}));
}