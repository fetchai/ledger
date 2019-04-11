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

#include "math/tensor.hpp"
#include "math/trigonometry.hpp"
#include <gtest/gtest.h>

template <typename T>
class TanhTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<fetch::math::Tensor<float>, fetch::math::Tensor<double>,
                                 fetch::math::Tensor<fetch::fixed_point::FixedPoint<32, 32>>>;
TYPED_TEST_CASE(TanhTest, MyTypes);

TYPED_TEST(TanhTest, tanh_22)
{
  TypeParam array1{{2, 2}};

  array1.Set({0, 0}, typename TypeParam::Type(0.3));
  array1.Set({0, 1}, typename TypeParam::Type(1.2));
  array1.Set({1, 0}, typename TypeParam::Type(0.7));
  array1.Set({1, 1}, typename TypeParam::Type(22));

  TypeParam output{{2, 2}};
  fetch::math::TanH(array1, output);

  TypeParam numpy_output{{2, 2}};

  numpy_output.Set({0, 0}, typename TypeParam::Type(0.29131261));
  numpy_output.Set({0, 1}, typename TypeParam::Type(0.83365461));
  numpy_output.Set({1, 0}, typename TypeParam::Type(0.60436778));
  numpy_output.Set({1, 1}, typename TypeParam::Type(1));

  ASSERT_TRUE(output.AllClose(numpy_output));
}
