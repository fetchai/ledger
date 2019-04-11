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
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType A = ArrayType(4);
  A.Set({0}, DataType(1));
  A.Set({1}, DataType(2));
  A.Set({2}, DataType(3));
  A.Set({3}, DataType(4));
  EXPECT_EQ(double(Euclidean(A, A)), 0);

  ArrayType B = ArrayType(4);
  B.Set({0}, DataType(1));
  B.Set({1}, DataType(2));
  B.Set({2}, DataType(3));
  B.Set({3}, DataType(2));

  EXPECT_EQ(double(Euclidean(A, B)), 2);
}

TYPED_TEST(EuclideanTest, matrix_euclidean_test)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType A = ArrayType({3, 4});
  A.Set({0, 0}, DataType(1));
  A.Set({0, 1}, DataType(2));
  A.Set({0, 2}, DataType(3));
  A.Set({0, 3}, DataType(4));

  A.Set({1, 0}, DataType(2));
  A.Set({1, 1}, DataType(3));
  A.Set({1, 2}, DataType(4));
  A.Set({1, 3}, DataType(5));

  A.Set({2, 0}, DataType(3));
  A.Set({2, 1}, DataType(4));
  A.Set({2, 2}, DataType(5));
  A.Set({2, 3}, DataType(6));

  ArrayType B = ArrayType({3, 4});
  B.Set({0, 0}, DataType(-1));
  B.Set({0, 1}, DataType(-2));
  B.Set({0, 2}, DataType(-3));
  B.Set({0, 3}, DataType(-4));

  B.Set({1, 0}, DataType(-2));
  B.Set({1, 1}, DataType(-3));
  B.Set({1, 2}, DataType(-4));
  B.Set({1, 3}, DataType(-5));

  B.Set({2, 0}, DataType(-3));
  B.Set({2, 1}, DataType(-4));
  B.Set({2, 2}, DataType(-5));
  B.Set({2, 3}, DataType(-6));

  ArrayType ret = EuclideanMatrix(A, B, 0);

  ASSERT_EQ(ret.shape().size(), 2);
  ASSERT_EQ(ret.shape().at(0), 1);
  ASSERT_EQ(ret.shape().at(1), 4);

  EXPECT_NEAR(double(ret.At(0)), 7.48331477, 1e-4);
  EXPECT_NEAR(double(ret.At(1)), 10.7703296, 1e-4);
  EXPECT_NEAR(double(ret.At(2)), 14.1421356, 1e-4);
  EXPECT_NEAR(double(ret.At(3)), 17.5499287, 1e-4);

  ret = EuclideanMatrix(A, B, 1);

  ASSERT_EQ(ret.shape().size(), 2);
  ASSERT_EQ(ret.shape().at(0), 3);
  ASSERT_EQ(ret.shape().at(1), 1);

  EXPECT_NEAR(double(ret.At(0)), 10.95445156, 1e-4);
  EXPECT_NEAR(double(ret.At(1)), 14.69693851, 1e-4);
  EXPECT_NEAR(double(ret.At(2)), 18.54723699, 1e-4);
}
