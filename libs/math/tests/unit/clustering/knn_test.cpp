//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/clustering/knn.hpp"
#include "math/distance/euclidean.hpp"
#include "test_types.hpp"

#include "gtest/gtest.h"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class ClusteringTest : public ::testing::Test
{
};

TYPED_TEST_CASE(ClusteringTest, TensorFloatingTypes);

TYPED_TEST(ClusteringTest, knn_euclidean_test)
{
  using ArrayType = TypeParam;
  using DataType  = typename TypeParam::Type;

  ArrayType A = ArrayType::FromString("1, 2, 3, 4; 2, 3, 4, 5; -1, -2, -3, -4; -2, -3, -4, -5");
  ArrayType v = ArrayType::FromString("3, 4, 5, 6");

  auto output = clustering::KNN<ArrayType, fetch::math::distance::Euclidean>(A, v, 4);

  EXPECT_EQ(output.at(0).first, SizeType(1));
  EXPECT_EQ(output.at(1).first, SizeType(0));
  EXPECT_EQ(output.at(2).first, SizeType(2));
  EXPECT_EQ(output.at(3).first, SizeType(3));

  EXPECT_NEAR(static_cast<double>(output.at(0).second), double(2),
              15.0 * static_cast<double>(function_tolerance<DataType>()));
  EXPECT_NEAR(static_cast<double>(output.at(1).second), double(4),
              15.0 * static_cast<double>(function_tolerance<DataType>()));
  EXPECT_NEAR(static_cast<double>(output.at(2).second), double(14.69694),
              15.0 * static_cast<double>(function_tolerance<DataType>()));
  EXPECT_NEAR(static_cast<double>(output.at(3).second), double(16.6132477),
              15.0 * static_cast<double>(function_tolerance<DataType>()));
}

TYPED_TEST(ClusteringTest, knn_cosine_test)
{
  using ArrayType = TypeParam;

  ArrayType A = ArrayType::FromString("1, 2, 3, 4; 2, 3, 4, 5; -1, -2, -3, -4; -2, -3, -4, -5");
  ArrayType v = ArrayType::FromString("3, 4, 5, 6");

  auto output = clustering::KNNCosine(A, v, 4);

  EXPECT_EQ(output.at(0).first, SizeType(1));
  EXPECT_NEAR(double(output.at(0).second), double(0.00215564), 1e-4);
  EXPECT_EQ(output.at(1).first, SizeType(0));
  EXPECT_NEAR(double(output.at(1).second), double(0.015626), 1e-4);
  EXPECT_EQ(output.at(2).first, SizeType(2));
  EXPECT_NEAR(double(output.at(2).second), double(1.98437), 1e-4);
  EXPECT_EQ(output.at(3).first, SizeType(3));
  EXPECT_NEAR(double(output.at(3).second), double(1.99784), 1e-4);
}

}  // namespace test
}  // namespace math
}  // namespace fetch
