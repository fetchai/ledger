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
#include "math/statistics/entropy.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {
template <typename T>
class EntropyTest : public ::testing::Test
{
};

TYPED_TEST_CASE(EntropyTest, TensorFloatingTypes);

TYPED_TEST(EntropyTest, entropy)
{
  using DataType  = typename TypeParam::Type;
  using ArrayType = TypeParam;

  ArrayType A(4);

  A.Set(SizeType{0}, fetch::math::Type<DataType>("0.1"));
  A.Set(SizeType{1}, fetch::math::Type<DataType>("0.2"));
  A.Set(SizeType{2}, fetch::math::Type<DataType>("0.3"));
  A.Set(SizeType{3}, fetch::math::Type<DataType>("0.4"));

  EXPECT_NEAR(double(statistics::Entropy(A)), 1.84643934467102,
              double(fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
