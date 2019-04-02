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

#include "math/distance/euclidean.hpp"
#include "math/tensor.hpp"

using namespace fetch::math::distance;
using namespace fetch::math;

template <typename T>
class EuclideanTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<16, 16>>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(EuclideanTest, MyTypes);

TYPED_TEST(EuclideanTest, simple_test)
{
  using DataType = typename TypeParam::Type;
  using SizeType = typename TypeParam::SizeType;

  TypeParam A{4};
  A.Set(SizeType{0}, DataType{1});
  A.Set(SizeType{1}, DataType{2});
  A.Set(SizeType{2}, DataType{3});
  A.Set(SizeType{3}, DataType{4});
  ASSERT_TRUE(Euclidean(A, A) == DataType{0});

  TypeParam B = TypeParam(4);
  B.Set(SizeType{0}, DataType{1});
  B.Set(SizeType{1}, DataType{2});
  B.Set(SizeType{2}, DataType{3});
  B.Set(SizeType{3}, DataType{2});

  ASSERT_TRUE(Euclidean(A, B) == DataType{2});
}
