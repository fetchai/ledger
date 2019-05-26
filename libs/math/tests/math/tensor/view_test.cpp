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

#include "vectorise/fixed_point/fixed_point.hpp"
#include "math/tensor.hpp"
#include "meta/type_traits.hpp"
#include <gtest/gtest.h>
#include <iomanip>
#include <iostream>

template <typename T>
class TensorViewTests : public ::testing::Test
{
};

using MyTypes = ::testing::Types<int32_t, uint32_t, int64_t, uint64_t, float, double,
                                 fetch::fixed_point::FixedPoint<32, 32>>;
TYPED_TEST_CASE(TensorViewTests, MyTypes);

using namespace fetch::math;

TYPED_TEST(TensorViewTests, size_test)
{
  TypeParam         from   = static_cast<TypeParam>(2);
  TypeParam         to     = static_cast<TypeParam>(50);
  TypeParam         step   = static_cast<TypeParam>(1);
  Tensor<TypeParam> tensor = Tensor<TypeParam>::Arange(from, to, step);
  tensor.Reshape({3, 16});

  EXPECT_EQ(tensor.height(), 3);
  EXPECT_EQ(tensor.width(), 16);

  auto view = tensor.View();
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view.width(), 16);

  tensor.Reshape({3, 8, 2});

  view = tensor.View();
  EXPECT_EQ(tensor.height(), 3);
  EXPECT_EQ(tensor.width(), 8);
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view.width(), 16);

  view = tensor.View(0);
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view.width(), 8);

  SizeType  counter = 0;
  TypeParam value   = from;
  for (auto &a : view)
  {
    EXPECT_EQ(a, value);
    ++counter;
    ++value;
  }

  EXPECT_EQ(counter, view.height() * view.width());
  view = tensor.View(1);
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view.width(), 8);

  for (auto &a : view)
  {
    EXPECT_EQ(a, value);
    ++counter;
    ++value;
  }

  EXPECT_EQ(counter, tensor.size());

  // Testing with vector notation
  view = tensor.View({0});
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view.width(), 8);
  value = 2;
  for (auto &a : view)
  {
    EXPECT_EQ(a, value);
    ++value;
  }

  view = tensor.View({1});
  EXPECT_EQ(view.height(), 3);
  EXPECT_EQ(view.width(), 8);
  for (auto &a : view)
  {
    EXPECT_EQ(a, value);
    ++value;
  }

  // Testing with vector notation
  value = 2;
  for (SizeType j = 0; j < 2; ++j)
  {
    for (SizeType i = 0; i < 8; ++i)
    {
      view = tensor.View({i, j});
      EXPECT_EQ(view.height(), 3);
      EXPECT_EQ(view.width(), 1);

      for (auto &a : view)
      {
        EXPECT_EQ(a, value);
        ++value;
      }
    }
  }
}


TYPED_TEST(TensorViewTests, data_layout)
{
  TypeParam         from   = static_cast<TypeParam>(2);
  TypeParam         to     = static_cast<TypeParam>(50);
  TypeParam         step   = static_cast<TypeParam>(1);
  Tensor<TypeParam> tensor = Tensor<TypeParam>::Arange(from, to, step);
  tensor.Reshape({3, 16});

  TypeParam value = from;

  for(uint64_t j=0; j < 16;++j)
  {
    auto view = tensor.View(j);

    uint64_t i = 0;
    for(auto &x: view.data())  
    {
      EXPECT_EQ(x, value);
      value += step;

      ++i;    
      if(i == 3)
      {
        break;
      }
    }
  }
  
}
