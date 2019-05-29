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

#include "math/fixed_point/fixed_point.hpp"
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
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({2, 4}));
  fetch::math::Tensor<TypeParam> t2(std::vector<std::uint64_t>({2, 4}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gtInput({0, 0, 6, -9, -3, 7, -14, -42});
  std::uint64_t    counter{0};
  for (SizeType i(0); i < 2; ++i)
  {
    for (SizeType j(0); j < 4; ++j)
    {
      t1.Set(i, j, TypeParam(t1Input[counter]));
      t2.Set(i, j, TypeParam(t2Input[counter]));
      ++counter;
    }
  }

  t1.InlineAdd(t2);

  counter = 0;
  for (std::uint64_t i(0); i < 2; ++i)
  {
    for (std::uint64_t j(0); j < 4; ++j)
    {
      EXPECT_EQ(t1.At(i, j), TypeParam(gtInput[counter]));
      EXPECT_EQ(t2.At(i, j), TypeParam(t2Input[counter]));
      ++counter;
    }
  }
}

TYPED_TEST(TensorOperationsTest, inline_mul_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({2, 4}));
  fetch::math::Tensor<TypeParam> t2(std::vector<std::uint64_t>({2, 4}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -11, -14});
  std::vector<int> gtInput({-1, -4, 9, 20, -40, -78, -77, 112});
  std::uint64_t    counter{0};
  for (SizeType i(0); i < 2; ++i)
  {
    for (SizeType j(0); j < 4; ++j)
    {
      t1.Set(i, j, TypeParam(t1Input[counter]));
      t2.Set(i, j, TypeParam(t2Input[counter]));
      ++counter;
    }
  }
  t1.InlineMultiply(t2);
  counter = 0;
  for (std::uint64_t i(0); i < 2; ++i)
  {
    for (std::uint64_t j(0); j < 4; ++j)
    {
      EXPECT_EQ(t1.At(i, j), TypeParam(gtInput[counter]));
      EXPECT_EQ(t2.At(i, j), TypeParam(t2Input[counter]));
      ++counter;
    }
  }
}

TYPED_TEST(TensorOperationsTest, sum_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({2, 4}));
  fetch::math::Tensor<TypeParam> t2(std::vector<std::uint64_t>({2, 4}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -11, -14});
  std::uint64_t    counter{0};
  for (SizeType i(0); i < 2; ++i)
  {
    for (SizeType j(0); j < 4; ++j)
    {
      t1.Set(i, j, TypeParam(t1Input[counter]));
      t2.Set(i, j, TypeParam(t2Input[counter]));
      ++counter;
    }
  }

  EXPECT_EQ(t1.Sum(), TypeParam(-4));
  EXPECT_EQ(t2.Sum(), TypeParam(-21));
}

TYPED_TEST(TensorOperationsTest, transpose_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({3, 5}));
  std::uint64_t                  counter{0};
  for (std::uint64_t i(0); i < 3; ++i)
  {
    for (std::uint64_t j(0); j < 5; ++j)
    {
      t1.At(i, j) = TypeParam(counter);
      counter++;
    }
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();

  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5, 3}));

  for (std::uint64_t i(0); i < 3; ++i)
  {
    for (std::uint64_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(t1.At(i, j), t2.At(j, i));
    }
  }
}

TYPED_TEST(TensorOperationsTest, transpose_untranspose_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({3, 5}));
  std::uint64_t                  counter{0};
  for (std::uint64_t i(0); i < 3; ++i)
  {
    for (std::uint64_t j(0); j < 5; ++j)
    {
      t1.At(i, j) = TypeParam(counter);
      counter++;
    }
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();
  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5, 3}));
  fetch::math::Tensor<TypeParam> t3 = t2.Transpose();
  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5, 3}));
  EXPECT_EQ(t3.shape(), std::vector<std::uint64_t>({3, 5}));

  counter = 0;
  for (std::uint64_t i(0); i < 3; ++i)
  {
    for (std::uint64_t j(0); j < 5; ++j)
    {
      EXPECT_EQ(t1.At(i, j), TypeParam(counter));
      EXPECT_EQ(t3.At(i, j), TypeParam(counter));
      ++counter;
    }
  }
}

TYPED_TEST(TensorOperationsTest, transpose_and_slice_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({3, 5}));
  std::uint64_t                  count = 0;
  for (std::uint64_t i{0}; i < 3; ++i)
  {
    for (std::uint64_t j{0}; j < 5; ++j)
    {
      t1.At(i, j) = TypeParam(count);
      ++count;
    }
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();

  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5, 3}));
  fetch::math::Tensor<TypeParam> t3 = t2.Slice(2).Copy();
  EXPECT_EQ(t3.shape(), std::vector<std::uint64_t>({1, 3}));

  EXPECT_EQ(t3.At(0, 0), TypeParam(2));
  EXPECT_EQ(t3.At(0, 1), TypeParam(7));
  EXPECT_EQ(t3.At(0, 2), TypeParam(12));
}

TYPED_TEST(TensorOperationsTest, slice_and_transpose_test)
{
  fetch::math::Tensor<TypeParam> t1(std::vector<std::uint64_t>({2, 3, 5}));
  std::uint64_t                  count = 0;
  for (std::uint64_t i{0}; i < 2; ++i)
  {
    for (std::uint64_t j{0}; j < 3; ++j)
    {
      for (std::uint64_t k{0}; k < 5; ++k)
      {
        t1.At(i, j, k) = TypeParam(count);
        ++count;
      }
    }
  }

  fetch::math::Tensor<TypeParam> t2 = t1.Slice(1).Copy();
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({1, 3, 5}));

  EXPECT_EQ(t2.At(0, 0, 0), TypeParam(15));
  EXPECT_EQ(t2.At(0, 1, 0), TypeParam(20));
  EXPECT_EQ(t2.At(0, 2, 0), TypeParam(25));
  EXPECT_EQ(t2.At(0, 0, 1), TypeParam(16));
  EXPECT_EQ(t2.At(0, 1, 1), TypeParam(21));
  EXPECT_EQ(t2.At(0, 2, 1), TypeParam(26));
  EXPECT_EQ(t2.At(0, 0, 2), TypeParam(17));
  EXPECT_EQ(t2.At(0, 1, 2), TypeParam(22));
  EXPECT_EQ(t2.At(0, 2, 2), TypeParam(27));
  EXPECT_EQ(t2.At(0, 0, 3), TypeParam(18));
  EXPECT_EQ(t2.At(0, 1, 3), TypeParam(23));
  EXPECT_EQ(t2.At(0, 2, 3), TypeParam(28));
  EXPECT_EQ(t2.At(0, 0, 4), TypeParam(19));
  EXPECT_EQ(t2.At(0, 1, 4), TypeParam(24));
  EXPECT_EQ(t2.At(0, 2, 4), TypeParam(29));

  fetch::math::Tensor<TypeParam> t3 = (t2.Squeeze()).Transpose();
  EXPECT_EQ(t3.shape(), std::vector<std::uint64_t>({5, 3}));

  // tensor is column major
  EXPECT_EQ(t3.At(0, 0), TypeParam(15));
  EXPECT_EQ(t3.At(1, 0), TypeParam(16));
  EXPECT_EQ(t3.At(2, 0), TypeParam(17));
  EXPECT_EQ(t3.At(3, 0), TypeParam(18));
  EXPECT_EQ(t3.At(4, 0), TypeParam(19));
  EXPECT_EQ(t3.At(0, 1), TypeParam(20));
  EXPECT_EQ(t3.At(1, 1), TypeParam(21));
  EXPECT_EQ(t3.At(2, 1), TypeParam(22));
  EXPECT_EQ(t3.At(3, 1), TypeParam(23));
  EXPECT_EQ(t3.At(4, 1), TypeParam(24));
  EXPECT_EQ(t3.At(0, 2), TypeParam(25));
  EXPECT_EQ(t3.At(1, 2), TypeParam(26));
  EXPECT_EQ(t3.At(2, 2), TypeParam(27));
  EXPECT_EQ(t3.At(3, 2), TypeParam(28));
  EXPECT_EQ(t3.At(4, 2), TypeParam(29));
}

// TODO (private 867) - reimplement shuffle & test
