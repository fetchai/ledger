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
#include <iostream>

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

TYPED_TEST(ClampTest, clamp_array_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;
  using SizeType  = typename TypeParam::SizeType;

  ArrayType A = ArrayType({6});

  A.Set(SizeType{0}, DataType(-10));
  A.Set(SizeType{1}, DataType(0));
  A.Set(SizeType{2}, DataType(1));
  A.Set(SizeType{3}, DataType(2));
  A.Set(SizeType{4}, DataType(3));
  A.Set(SizeType{5}, DataType(10));

  // Expected results
  ArrayType A_clamp_expected = ArrayType({6});
  A_clamp_expected.Set(SizeType{0}, DataType(2));
  A_clamp_expected.Set(SizeType{1}, DataType(2));
  A_clamp_expected.Set(SizeType{2}, DataType(2));
  A_clamp_expected.Set(SizeType{3}, DataType(2));
  A_clamp_expected.Set(SizeType{4}, DataType(3));
  A_clamp_expected.Set(SizeType{5}, DataType(3));

  // Compare results with expected results
  ArrayType A_norm = Clamp(A, DataType(2), DataType(3));

  for (SizeType i{0}; i < A.size(); i++)
  {
    EXPECT_NEAR(double(A_norm.At(i)), double(A_clamp_expected.At(i)), 1e-4);
  }
}