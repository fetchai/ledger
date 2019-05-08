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

#include "math/distance/cosine.hpp"
#include "math/tensor.hpp"

using namespace fetch::math::distance;
using namespace fetch::math;

template <typename T>
class DistanceTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(DistanceTest, MyTypes);

TYPED_TEST(DistanceTest, cosine_distance)
{

  using DataType  = typename TypeParam::Type;
  using SizeType  = typename TypeParam::SizeType;
  using ArrayType = TypeParam;

  ArrayType A = ArrayType({1, 4});
  A.Set(SizeType{0}, SizeType{0}, DataType(1));
  A.Set(SizeType{0}, SizeType{1}, DataType(2));
  A.Set(SizeType{0}, SizeType{2}, DataType(3));
  A.Set(SizeType{0}, SizeType{3}, DataType(4));

  ArrayType B = ArrayType({1, 4});
  B.Set(SizeType{0}, SizeType{0}, DataType(-1));
  B.Set(SizeType{0}, SizeType{1}, DataType(-2));
  B.Set(SizeType{0}, SizeType{2}, DataType(-3));
  B.Set(SizeType{0}, SizeType{3}, DataType(-4));

  EXPECT_NEAR(double(Cosine(A, A)), 0, (double)fetch::math::meta::tolerance<DataType>());
  EXPECT_NEAR(double(Cosine(A, B)), 2, (double)fetch::math::meta::tolerance<DataType>());

  ArrayType C = ArrayType({1, 4});
  C.Set(SizeType{0}, SizeType{0}, DataType(1));
  C.Set(SizeType{0}, SizeType{1}, DataType(2));
  C.Set(SizeType{0}, SizeType{2}, DataType(3));
  C.Set(SizeType{0}, SizeType{3}, DataType(2));

  EXPECT_NEAR(double(Cosine(A, C)), double(1.0) - double(0.94672926240625754),
              (double)fetch::math::meta::tolerance<DataType>());
}
