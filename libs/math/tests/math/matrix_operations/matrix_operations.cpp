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

#include "core/fixed_point/fixed_point.hpp"
#include "core/random/lcg.hpp"
#include "math/matrix_operations.hpp"
#include "math/tensor.hpp"

#include <chrono>

template <typename T>
class FreeFunctionsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(FreeFunctionsTest, MyTypes);

TYPED_TEST(FreeFunctionsTest, ArgMax_OneDimension)
{
  TypeParam array1{4};

  array1.Set(0, typename TypeParam::Type(0.3));
  array1.Set(1, typename TypeParam::Type(1.2));
  array1.Set(2, typename TypeParam::Type(0.7));
  array1.Set(3, typename TypeParam::Type(22));

  TypeParam output{1};
  fetch::math::ArgMax(array1, output);
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(3));

  array1.Set(3, typename TypeParam::Type(0));
  fetch::math::ArgMax(array1, output);
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(1));

  array1.Set(1, typename TypeParam::Type(0));
  fetch::math::ArgMax(array1, output);
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(2));
}

TYPED_TEST(FreeFunctionsTest, ArgMax_TwoDimension)
{
  using SizeType = typename TypeParam::SizeType;

  SizeType  n_data     = 4;
  SizeType  n_features = 2;
  TypeParam array1{{n_data, n_features}};

  array1.Set({0, 0}, typename TypeParam::Type(-17));
  array1.Set({0, 1}, typename TypeParam::Type(21));
  array1.Set({1, 0}, typename TypeParam::Type(0));
  array1.Set({1, 1}, typename TypeParam::Type(0));
  array1.Set({2, 0}, typename TypeParam::Type(13));
  array1.Set({2, 1}, typename TypeParam::Type(999));
  array1.Set({3, 0}, typename TypeParam::Type(21));
  array1.Set({3, 1}, typename TypeParam::Type(-0.5));

  TypeParam output{n_data};
  fetch::math::ArgMax(array1, output, SizeType(1));
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(1));
  EXPECT_EQ(output.At(1), typename TypeParam::SizeType(0));
  EXPECT_EQ(output.At(2), typename TypeParam::SizeType(1));
  EXPECT_EQ(output.At(3), typename TypeParam::SizeType(0));
}

TYPED_TEST(FreeFunctionsTest, ArgMax_TwoDimension_off_axis)
{
  using SizeType = typename TypeParam::SizeType;

  SizeType  n_data     = 4;
  SizeType  n_features = 2;
  TypeParam array1{{n_data, n_features}};

  array1.Set({0, 0}, typename TypeParam::Type(-17));
  array1.Set({0, 1}, typename TypeParam::Type(21));
  array1.Set({1, 0}, typename TypeParam::Type(0));
  array1.Set({1, 1}, typename TypeParam::Type(0));
  array1.Set({2, 0}, typename TypeParam::Type(13));
  array1.Set({2, 1}, typename TypeParam::Type(999));
  array1.Set({3, 0}, typename TypeParam::Type(21));
  array1.Set({3, 1}, typename TypeParam::Type(-0.5));

  TypeParam output{n_features};
  fetch::math::ArgMax(array1, output, SizeType(0));
  EXPECT_EQ(output.At(0), typename TypeParam::SizeType(3));
  EXPECT_EQ(output.At(1), typename TypeParam::SizeType(2));
}