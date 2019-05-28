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

#include "math/clustering/knn.hpp"
#include "math/distance/euclidean.hpp"
#include "math/tensor.hpp"

using namespace fetch::math;

template <typename T>
class ClusteringTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;

TYPED_TEST_CASE(ClusteringTest, MyTypes);

TYPED_TEST(ClusteringTest, knn_euclidean_test)
{
  using DataType     = typename TypeParam::Type;
  using ArrayType    = TypeParam;
  using SizeTypeHere = typename TypeParam::SizeType;

  ArrayType A = ArrayType({4, 4});

  A.Set(SizeTypeHere{0}, SizeTypeHere{0}, DataType(1));
  A.Set(SizeTypeHere{0}, SizeTypeHere{1}, DataType(2));
  A.Set(SizeTypeHere{0}, SizeTypeHere{2}, DataType(3));
  A.Set(SizeTypeHere{0}, SizeTypeHere{3}, DataType(4));

  A.Set(SizeTypeHere{1}, SizeTypeHere{0}, DataType(2));
  A.Set(SizeTypeHere{1}, SizeTypeHere{1}, DataType(3));
  A.Set(SizeTypeHere{1}, SizeTypeHere{2}, DataType(4));
  A.Set(SizeTypeHere{1}, SizeTypeHere{3}, DataType(5));

  A.Set(SizeTypeHere{2}, SizeTypeHere{0}, DataType(-1));
  A.Set(SizeTypeHere{2}, SizeTypeHere{1}, DataType(-2));
  A.Set(SizeTypeHere{2}, SizeTypeHere{2}, DataType(-3));
  A.Set(SizeTypeHere{2}, SizeTypeHere{3}, DataType(-4));

  A.Set(SizeTypeHere{3}, SizeTypeHere{0}, DataType(-2));
  A.Set(SizeTypeHere{3}, SizeTypeHere{1}, DataType(-3));
  A.Set(SizeTypeHere{3}, SizeTypeHere{2}, DataType(-4));
  A.Set(SizeTypeHere{3}, SizeTypeHere{3}, DataType(-5));

  ArrayType v = ArrayType({1, 4});

  v.Set(SizeTypeHere{0}, SizeTypeHere{0}, DataType(3));
  v.Set(SizeTypeHere{0}, SizeTypeHere{1}, DataType(4));
  v.Set(SizeTypeHere{0}, SizeTypeHere{2}, DataType(5));
  v.Set(SizeTypeHere{0}, SizeTypeHere{3}, DataType(6));

  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output =
      fetch::math::clustering::KNN<ArrayType, fetch::math::distance::Euclidean>(A, v, 4);

  EXPECT_EQ(output.at(0).first, SizeTypeHere(1));
  EXPECT_NEAR(double(output.at(0).second), double(2), 1e-4);
  EXPECT_EQ(output.at(1).first, SizeTypeHere(0));
  EXPECT_NEAR(double(output.at(1).second), double(4), 1e-4);
  EXPECT_EQ(output.at(2).first, SizeTypeHere(2));
  EXPECT_NEAR(double(output.at(2).second), double(14.6969384), 1e-4);
  EXPECT_EQ(output.at(3).first, SizeTypeHere(3));
  EXPECT_NEAR(double(output.at(3).second), double(16.6132477), 1e-4);
}

TYPED_TEST(ClusteringTest, knn_cosine_test)
{
  using DataType     = typename TypeParam::Type;
  using ArrayType    = TypeParam;
  using SizeTypeHere = typename TypeParam::SizeType;

  ArrayType A = ArrayType({4, 4});

  A.Set(SizeTypeHere{0}, SizeTypeHere{0}, DataType(1));
  A.Set(SizeTypeHere{0}, SizeTypeHere{1}, DataType(2));
  A.Set(SizeTypeHere{0}, SizeTypeHere{2}, DataType(3));
  A.Set(SizeTypeHere{0}, SizeTypeHere{3}, DataType(4));

  A.Set(SizeTypeHere{1}, SizeTypeHere{0}, DataType(2));
  A.Set(SizeTypeHere{1}, SizeTypeHere{1}, DataType(3));
  A.Set(SizeTypeHere{1}, SizeTypeHere{2}, DataType(4));
  A.Set(SizeTypeHere{1}, SizeTypeHere{3}, DataType(5));

  A.Set(SizeTypeHere{2}, SizeTypeHere{0}, DataType(-1));
  A.Set(SizeTypeHere{2}, SizeTypeHere{1}, DataType(-2));
  A.Set(SizeTypeHere{2}, SizeTypeHere{2}, DataType(-3));
  A.Set(SizeTypeHere{2}, SizeTypeHere{3}, DataType(-4));

  A.Set(SizeTypeHere{3}, SizeTypeHere{0}, DataType(-2));
  A.Set(SizeTypeHere{3}, SizeTypeHere{1}, DataType(-3));
  A.Set(SizeTypeHere{3}, SizeTypeHere{2}, DataType(-4));
  A.Set(SizeTypeHere{3}, SizeTypeHere{3}, DataType(-5));

  ArrayType v = ArrayType({1, 4});

  v.Set(SizeTypeHere{0}, SizeTypeHere{0}, DataType(3));
  v.Set(SizeTypeHere{0}, SizeTypeHere{1}, DataType(4));
  v.Set(SizeTypeHere{0}, SizeTypeHere{2}, DataType(5));
  v.Set(SizeTypeHere{0}, SizeTypeHere{3}, DataType(6));

  std::vector<std::pair<typename ArrayType::SizeType, typename ArrayType::Type>> output =
      fetch::math::clustering::KNNCosine(A, v, 4);

  EXPECT_EQ(output.at(0).first, SizeTypeHere(1));
  EXPECT_NEAR(double(output.at(0).second), double(0.00215564), 1e-4);
  EXPECT_EQ(output.at(1).first, SizeTypeHere(0));
  EXPECT_NEAR(double(output.at(1).second), double(0.015626), 1e-4);
  EXPECT_EQ(output.at(2).first, SizeTypeHere(2));
  EXPECT_NEAR(double(output.at(2).second), double(1.98437), 1e-4);
  EXPECT_EQ(output.at(3).first, SizeTypeHere(3));
  EXPECT_NEAR(double(output.at(3).second), double(1.99784), 1e-4);
}
