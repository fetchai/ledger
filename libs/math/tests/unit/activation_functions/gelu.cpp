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
#include "math/activation_functions/gelu.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class GeluTest : public ::testing::Test
{
};

TYPED_TEST_CASE(GeluTest, TensorFloatingTypes);

TYPED_TEST(GeluTest, exact_value_test)
{
  using DataType = typename TypeParam::Type;

  TypeParam input = TypeParam::FromString("-10, -2, -1, -0.5, 0, 0.2, 1.6, 5.7, 12");
  TypeParam gt    = TypeParam::FromString(
      "-0.0000000000, -0.0454022884, -0.1588079929, -0.1542859972, 0.0000000000,  0.1158514246,  "
      "1.5121370554,  5.6999998093, 12.0000000000");

  TypeParam output = fetch::math::Gelu(input);
  std::cout << "output.ToString(): " << output.ToString() << std::endl;
  ASSERT_TRUE(output.AllClose(
      gt, fetch::math::function_tolerance<DataType>(),
      fetch::math::Type<DataType>("2.8") * fetch::math::function_tolerance<DataType>()));
}

}  // namespace test
}  // namespace math
}  // namespace fetch
