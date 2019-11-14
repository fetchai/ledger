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

#include "gtest/gtest.h"
#include "math/tensor.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class TensorConstructorTest : public ::testing::Test
{
};

TYPED_TEST_CASE(TensorConstructorTest, FloatIntAndUIntTypes);

TYPED_TEST(TensorConstructorTest, string_construction)
{
  auto tensor = fetch::math::Tensor<TypeParam>::FromString(R"(1 3 4)");
  ASSERT_EQ(tensor.At(0, 0), TypeParam(1));
  ASSERT_EQ(tensor.At(0, 1), TypeParam(3));
  ASSERT_EQ(tensor.At(0, 2), TypeParam(4));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
