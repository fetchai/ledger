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

#include "core/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include "meta/type_traits.hpp"
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

template <typename T>
class TensorViewTests : public ::testing::Test
{
};

using MyTypes = ::testing::Types<int, unsigned int, long, unsigned long, float, double,
                                 fetch::fixed_point::FixedPoint<32, 32>>;
TYPED_TEST_CASE(TensorViewTests, MyTypes);


using namespace fetch::math;


TYPED_TEST(TensorViewTests, size_test)
{
  TypeParam from = static_cast<TypeParam>(2);
  TypeParam to   = static_cast<TypeParam>(50);
  TypeParam step = static_cast<TypeParam>(1);
  Tensor< TypeParam > tensor = Tensor<TypeParam>::Arange(from, to, step);
  tensor.Reshape({3,16});

  EXPECT_EQ(tensor.height(), 3);
  EXPECT_EQ(tensor.width(), 16);

  auto view = tensor.View();
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view.width(), 16);  

  tensor.Reshape({3,8,2});

  view = tensor.View();
  EXPECT_EQ(tensor.height(), 3);
  EXPECT_EQ(tensor.width(), 8);
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view.width(), 16);  

  view = tensor.View(1);
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view.width(), 8);  

}