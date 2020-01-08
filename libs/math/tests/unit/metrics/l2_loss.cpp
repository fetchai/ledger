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
#include "math/metrics/l2_loss.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class L2LossTest : public ::testing::Test
{
};

TYPED_TEST_CASE(L2LossTest, TensorFloatingTypes);

TYPED_TEST(L2LossTest, value_test)
{
  TypeParam test_array = TypeParam::FromString("1, -2, 3, -4, 5, -6, 7, -8");

  // initialise to non-zero just to avoid correct value at initialisation
  typename TypeParam::Type score(0);
  score = fetch::math::L2Loss(test_array);

  // test correct values
  EXPECT_NEAR(double(score), double(102), 1e-7);
}

}  // namespace test
}  // namespace math
}  // namespace fetch
