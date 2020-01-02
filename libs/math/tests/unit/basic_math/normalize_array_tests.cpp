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
#include "math/normalize_array.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class NormalizeArrayTest : public ::testing::Test
{
};

TYPED_TEST_CASE(NormalizeArrayTest, TensorFloatingTypes);

TYPED_TEST(NormalizeArrayTest, conditional_distance)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType A = ArrayType({4});

  A.Set(SizeType{0}, static_cast<DataType>(1));
  A.Set(SizeType{1}, DataType{2});
  A.Set(SizeType{2}, DataType{3});
  A.Set(SizeType{3}, DataType{4});

  // Expected results
  ArrayType A_norm_expected = ArrayType({4});
  A_norm_expected.Set(SizeType{0}, fetch::math::Type<DataType>("0.1"));
  A_norm_expected.Set(SizeType{1}, fetch::math::Type<DataType>("0.2"));
  A_norm_expected.Set(SizeType{2}, fetch::math::Type<DataType>("0.3"));
  A_norm_expected.Set(SizeType{3}, fetch::math::Type<DataType>("0.4"));

  // Compare results with expected results
  ArrayType A_norm = NormalizeArray(A);
  for (SizeType i{0}; i < A.size(); i++)
  {
    EXPECT_NEAR(double(A_norm.At(i)), double(A_norm_expected.At(i)), 1e-4);
  }
}

}  // namespace test
}  // namespace math
}  // namespace fetch
