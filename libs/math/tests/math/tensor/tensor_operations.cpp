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
#include <gtest/gtest.h>

template <typename T>
class TensorOperationsTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<int, long, float, double, fetch::fixed_point::FixedPoint<16, 16>,
                                 fetch::fixed_point::FixedPoint<32, 32>>;
TYPED_TEST_CASE(TensorOperationsTest, MyTypes);

TYPED_TEST(TensorOperationsTest, inline_add_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({3, 5}));
  fetch::math::Tensor<TypeParam> t2(std::vector<std::uint64_t>({3, 5}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gtInput({0, 0, 6, -9, -3, 7, -14, -42});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    t1.Set(i, TypeParam(t1Input[i]));
    t2.Set(i, TypeParam(t2Input[i]));
  }
  t1.InlineAdd(t2);
  for (std::uint64_t i(0); i < 8; ++i)
  {
    EXPECT_EQ(t1.At(i), TypeParam(gtInput[i]));
    EXPECT_EQ(t2.At(i), TypeParam(t2Input[i]));
  }
}

TYPED_TEST(TensorOperationsTest, inline_add_with_stride_test)
{
  fetch::math::Tensor<TypeParam> t1({3, 5});
  fetch::math::Tensor<TypeParam> t2({3, 5}, {12, 4});

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gtInput({0, 0, 6, -9, -3, 7, -14, -42});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    t1.Set(i, TypeParam(t1Input[i]));
    t2.Set(i, TypeParam(t2Input[i]));
  }
  t1.InlineAdd(t2);
  for (std::uint64_t i(0); i < 8; ++i)
  {
    EXPECT_EQ(t1.At(i), TypeParam(gtInput[i]));
    EXPECT_EQ(t2.At(i), TypeParam(t2Input[i]));
  }
}

TYPED_TEST(TensorOperationsTest, inline_mul_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({3, 5}));
  fetch::math::Tensor<TypeParam> t2(std::vector<std::uint64_t>({3, 5}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -11, -14});
  std::vector<int> gtInput({-1, -4, 9, 20, -40, -78, -77, 112});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    t1.Set(i, TypeParam(t1Input[i]));
    t2.Set(i, TypeParam(t2Input[i]));
  }
  t1.InlineMultiply(t2);
  for (std::uint64_t i(0); i < 8; ++i)
  {
    EXPECT_EQ(t1.At(i), TypeParam(gtInput[i]));
    EXPECT_EQ(t2.At(i), TypeParam(t2Input[i]));
  }
}

TYPED_TEST(TensorOperationsTest, sum_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({3, 5}));
  fetch::math::Tensor<TypeParam> t2(std::vector<std::uint64_t>({3, 5}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -11, -14});
  for (std::uint64_t i(0); i < 8; ++i)
  {
    t1.Set(i, TypeParam(t1Input[i]));
    t2.Set(i, TypeParam(t2Input[i]));
  }

  EXPECT_EQ(t1.Sum(), TypeParam(-4));
  EXPECT_EQ(t2.Sum(), TypeParam(-21));
}

TYPED_TEST(TensorOperationsTest, transpose_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({3, 5}));
  for (std::uint64_t i(0); i < t1.size(); ++i)
  {
    t1.At(i) = TypeParam(i);
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();

  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5, 3}));

  for (std::uint64_t i(0); i < 3; ++i)
  {
    for (std::uint64_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(t1.Get({i, j}), t2.Get({j, i}));
    }
  }
}

TYPED_TEST(TensorOperationsTest, transpose_with_stride_test)
{
  fetch::math::Tensor<TypeParam> t1({3, 5}, {2, 3});
  for (std::uint64_t i(0); i < t1.size(); ++i)
  {
    t1.At(i) = TypeParam(i);
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();

  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5, 3}));

  for (std::uint64_t i(0); i < 3; ++i)
  {
    for (std::uint64_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(t1.Get({i, j}), t2.Get({j, i}));
    }
  }
}

TYPED_TEST(TensorOperationsTest, transpose_untranspose_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({3, 5}));
  for (std::uint64_t i(0); i < t1.size(); ++i)
  {
    t1.At(i) = TypeParam(i);
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();
  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5, 3}));
  fetch::math::Tensor<TypeParam> t3 = t2.Transpose();
  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5, 3}));
  EXPECT_EQ(t3.shape(), std::vector<std::uint64_t>({3, 5}));

  for (std::uint64_t i(0); i < t1.size(); ++i)
  {
    EXPECT_EQ(t1.At(i), TypeParam(i));
    EXPECT_EQ(t3.At(i), TypeParam(i));
  }
}

TYPED_TEST(TensorOperationsTest, transpose_and_slice_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({3, 5}));
  for (std::uint64_t i(0); i < t1.size(); ++i)
  {
    t1.At(i) = TypeParam(i);
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5, 3}));
  fetch::math::Tensor<TypeParam> t3 = t2.Slice(2);
  EXPECT_EQ(t3.shape(), std::vector<std::uint64_t>({3}));

  EXPECT_EQ(t3.At(0), TypeParam(2));
  EXPECT_EQ(t3.At(1), TypeParam(7));
  EXPECT_EQ(t3.At(2), TypeParam(12));
}

TYPED_TEST(TensorOperationsTest, slice_and_transpose_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({2, 3, 5}));
  for (std::uint64_t i(0); i < t1.size(); ++i)
  {
    t1.At(i) = TypeParam(i);
  }

  fetch::math::Tensor<TypeParam> t2 = t1.Slice(1);
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({3, 5}));

  fetch::math::Tensor<TypeParam> t3 = t2.Transpose();
  EXPECT_EQ(t3.shape(), std::vector<std::uint64_t>({5, 3}));

  EXPECT_EQ(t3.At(0), TypeParam(15));
  EXPECT_EQ(t3.At(1), TypeParam(20));
  EXPECT_EQ(t3.At(2), TypeParam(25));
  EXPECT_EQ(t3.At(3), TypeParam(16));
  EXPECT_EQ(t3.At(4), TypeParam(21));
  EXPECT_EQ(t3.At(5), TypeParam(26));
  EXPECT_EQ(t3.At(6), TypeParam(17));
  EXPECT_EQ(t3.At(7), TypeParam(22));
  EXPECT_EQ(t3.At(8), TypeParam(27));
  EXPECT_EQ(t3.At(9), TypeParam(18));
  EXPECT_EQ(t3.At(10), TypeParam(23));
  EXPECT_EQ(t3.At(11), TypeParam(28));
  EXPECT_EQ(t3.At(12), TypeParam(19));
  EXPECT_EQ(t3.At(13), TypeParam(24));
  EXPECT_EQ(t3.At(14), TypeParam(29));
}
