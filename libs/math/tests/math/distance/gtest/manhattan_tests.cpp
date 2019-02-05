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

#include <iomanip>
#include <iostream>

#include "core/random/lcg.hpp"
#include <gtest/gtest.h>
#include <math/distance/manhattan.hpp>

using namespace fetch::math::distance;
using namespace fetch::math;

TEST(manhattan_gtest, basic_info)
{
  ShapelessArray<double> A = ShapelessArray<double>(3);
  A.Set({0}, 1);
  A.Set({1}, 0);
  A.Set({2}, 0);
  EXPECT_EQ(0, Manhattan(A, A));

  ShapelessArray<double> B = ShapelessArray<double>(3);
  B.Set({0}, 0);
  B.Set({1}, 1);
  B.Set({2}, 0);
  EXPECT_EQ(Manhattan(A, B), 2);

  B.Set({0}, 0);
  B.Set({1}, 2);
  B.Set({2}, 0);
  EXPECT_EQ(Manhattan(A, B), 3);

  B.Set({0}, 1);
  B.Set({1}, 1);
  B.Set({2}, 0);
  EXPECT_EQ(Manhattan(A, B), 1);
}
