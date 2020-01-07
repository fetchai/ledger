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

#include "gtest/gtest.h"
#include "math/base_types.hpp"
#include "math/distance/cosine.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class DistanceTest : public ::testing::Test
{
};

TYPED_TEST_CASE(DistanceTest, TensorFloatingTypes);

TYPED_TEST(DistanceTest, cosine_distance)
{

  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType A = ArrayType({1, 4});
  A.Set(SizeType{0}, SizeType{0}, static_cast<DataType>(1));
  A.Set(SizeType{0}, SizeType{1}, DataType(2));
  A.Set(SizeType{0}, SizeType{2}, DataType(3));
  A.Set(SizeType{0}, SizeType{3}, DataType(4));

  ArrayType B = ArrayType({1, 4});
  B.Set(SizeType{0}, SizeType{0}, DataType(-1));
  B.Set(SizeType{0}, SizeType{1}, DataType(-2));
  B.Set(SizeType{0}, SizeType{2}, DataType(-3));
  B.Set(SizeType{0}, SizeType{3}, DataType(-4));

  EXPECT_NEAR(double(distance::Cosine(A, A)), 0, (double)function_tolerance<DataType>());
  EXPECT_NEAR(double(distance::Cosine(A, B)), 2, (double)function_tolerance<DataType>());

  ArrayType C = ArrayType({1, 4});
  C.Set(SizeType{0}, SizeType{0}, static_cast<DataType>(1));
  C.Set(SizeType{0}, SizeType{1}, DataType(2));
  C.Set(SizeType{0}, SizeType{2}, DataType(3));
  C.Set(SizeType{0}, SizeType{3}, DataType(2));

  EXPECT_NEAR(double(distance::Cosine(A, C)), double(1.0) - double(0.94672926240625754),
              (double)function_tolerance<DataType>());
}

}  // namespace test
}  // namespace math
}  // namespace fetch
