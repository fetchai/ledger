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

#include "math/tensor.hpp"
#include <gtest/gtest.h>

template <typename T>
class TensorOperationsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<char, int, long, float, double>;
TYPED_TEST_CASE(TensorOperationsTest, MyTypes);

TYPED_TEST(TensorOperationsTest, inline_add_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<size_t>({3, 5}));
  fetch::math::Tensor<TypeParam> t2(std::vector<size_t>({3, 5}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gtInput({0, 0, 6, -9, -3, 7, -14, -42});
  for (size_t i(0); i < 8; ++i)
  {
    t1.Set(i, TypeParam(t1Input[i]));
    t2.Set(i, TypeParam(t2Input[i]));
  }
  t1.Add_(t2);
  for (size_t i(0); i < 8; ++i)
  {
    EXPECT_EQ(t1.At(i), gtInput[i]);
    EXPECT_EQ(t2.At(i), t2Input[i]);
  }
}

TYPED_TEST(TensorOperationsTest, inline_add_with_stride_test)
{
  fetch::math::Tensor<TypeParam> t1({3, 5});
  fetch::math::Tensor<TypeParam> t2({3, 5}, {12, 4});

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gtInput({0, 0, 6, -9, -3, 7, -14, -42});
  for (size_t i(0); i < 8; ++i)
  {
    t1.Set(i, TypeParam(t1Input[i]));
    t2.Set(i, TypeParam(t2Input[i]));
  }
  t1.Add_(t2);
  for (size_t i(0); i < 8; ++i)
  {
    EXPECT_EQ(t1.At(i), gtInput[i]);
    EXPECT_EQ(t2.At(i), t2Input[i]);
  }
}

TYPED_TEST(TensorOperationsTest, inline_mul_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<size_t>({3, 5}));
  fetch::math::Tensor<TypeParam> t2(std::vector<size_t>({3, 5}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -11, -14});
  std::vector<int> gtInput({-1, -4, 9, 20, -40, -78, -77, 112});
  for (size_t i(0); i < 8; ++i)
  {
    t1.Set(i, TypeParam(t1Input[i]));
    t2.Set(i, TypeParam(t2Input[i]));
  }
  t1.Mul_(t2);
  for (size_t i(0); i < 8; ++i)
  {
    EXPECT_EQ(t1.At(i), gtInput[i]);
    EXPECT_EQ(t2.At(i), t2Input[i]);
  }
}

TYPED_TEST(TensorOperationsTest, sum_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<size_t>({3, 5}));
  fetch::math::Tensor<TypeParam> t2(std::vector<size_t>({3, 5}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -11, -14});
  for (size_t i(0); i < 8; ++i)
  {
    t1.Set(i, TypeParam(t1Input[i]));
    t2.Set(i, TypeParam(t2Input[i]));
  }

  EXPECT_EQ(t1.Sum(), TypeParam(-4));
  EXPECT_EQ(t2.Sum(), TypeParam(-21));
}
