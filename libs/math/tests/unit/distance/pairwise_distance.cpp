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
#include "math/distance/pairwise_distance.hpp"
#include "math/matrix_operations.hpp"
#include "test_types.hpp"

namespace fetch {
namespace math {
namespace test {

template <typename T>
class PairWiseDistanceTest : public ::testing::Test
{
};

TYPED_TEST_CASE(PairWiseDistanceTest, TensorFloatingTypes);

TYPED_TEST(PairWiseDistanceTest, simple_test)
{
  TypeParam data = TypeParam::FromString("0, 1, 2; 3, 4, 5; 6, 7, 8");
  TypeParam gt   = TypeParam::FromString("-9, -18, -9");

  TypeParam R = TypeParam({1, data.shape(0) * (data.shape(0) - 1) / 2});

  distance::PairWiseDistance(data,
                             [](TypeParam x, TypeParam y) -> typename TypeParam::Type {
                               TypeParam z = x - y;
                               return fetch::math::Sum(z);
                             },
                             R);

  EXPECT_TRUE(R.AllClose(gt));

  data = TypeParam::FromString("1, 1, 1, 1; 1, 1, 1, 1");
  gt   = TypeParam::FromString("0");

  R = TypeParam({1, data.shape(0) * (data.shape(0) - 1) / 2});

  distance::PairWiseDistance(data,
                             [](TypeParam x, TypeParam y) -> typename TypeParam::Type {
                               TypeParam z = x - y;
                               return fetch::math::Sum(z);
                             },
                             R);

  EXPECT_TRUE(R.AllClose(gt));

  data = TypeParam::FromString("1, 2, 3; 4, 5, 6; 1, 2, 3");
  gt   = TypeParam::FromString("-9, 0, 9");

  R = TypeParam({1, data.shape(0) * (data.shape(0) - 1) / 2});

  distance::PairWiseDistance(data,
                             [](TypeParam x, TypeParam y) -> typename TypeParam::Type {
                               TypeParam z = x - y;
                               return fetch::math::Sum(z);
                             },
                             R);

  EXPECT_TRUE(R.AllClose(gt));
}
}  // namespace test
}  // namespace math
}  // namespace fetch
