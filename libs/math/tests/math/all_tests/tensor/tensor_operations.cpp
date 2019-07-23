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
#include "vectorise/fixed_point/fixed_point.hpp"

#include "gtest/gtest.h"

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

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({2, 4}));
  fetch::math::Tensor<TypeParam> t2(std::vector<SizeType>({2, 4}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -21, -34});
  std::vector<int> gtInput({0, 0, 6, -9, -3, 7, -14, -42});
  SizeType         counter{0};
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
  for (SizeType i(0); i < 2; ++i)
  {
    for (SizeType j(0); j < 4; ++j)
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

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({2, 4}));
  fetch::math::Tensor<TypeParam> t2(std::vector<SizeType>({2, 4}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -11, -14});
  std::vector<int> gtInput({-1, -4, 9, 20, -40, -78, -77, 112});
  SizeType         counter{0};
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
  for (SizeType i(0); i < 2; ++i)
  {
    for (SizeType j(0); j < 4; ++j)
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

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({2, 4}));
  fetch::math::Tensor<TypeParam> t2(std::vector<SizeType>({2, 4}));

  std::vector<int> t1Input({1, -2, 3, -4, 5, -6, 7, -8});
  std::vector<int> t2Input({-1, 2, 3, -5, -8, 13, -11, -14});
  SizeType         counter{0};
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
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({3, 5}));
  SizeType                       counter{0};
  for (SizeType i(0); i < 3; ++i)
  {
    for (SizeType j(0); j < 5; ++j)
    {
      t1.At(i, j) = TypeParam(counter);
      counter++;
    }
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();

  EXPECT_EQ(t1.shape(), std::vector<SizeType>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<SizeType>({5, 3}));

  for (SizeType i(0); i < 3; ++i)
  {
    for (SizeType j(0); j < 5; ++j)
    {
      EXPECT_EQ(t1.At(i, j), t2.At(j, i));
    }
  }
}

TYPED_TEST(TensorOperationsTest, transpose_untranspose_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({3, 5}));
  SizeType                       counter{0};
  for (SizeType i(0); i < 3; ++i)
  {
    for (SizeType j(0); j < 5; ++j)
    {
      t1.At(i, j) = TypeParam(counter);
      counter++;
    }
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();
  EXPECT_EQ(t1.shape(), std::vector<SizeType>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<SizeType>({5, 3}));
  fetch::math::Tensor<TypeParam> t3 = t2.Transpose();
  EXPECT_EQ(t1.shape(), std::vector<SizeType>({3, 5}));
  EXPECT_EQ(t2.shape(), std::vector<SizeType>({5, 3}));
  EXPECT_EQ(t3.shape(), std::vector<SizeType>({3, 5}));

  counter = 0;
  for (SizeType i(0); i < 3; ++i)
  {
    for (SizeType j(0); j < 5; ++j)
    {
      EXPECT_EQ(t1.At(i, j), TypeParam(counter));
      EXPECT_EQ(t3.At(i, j), TypeParam(counter));
      ++counter;
    }
  }
}

TYPED_TEST(TensorOperationsTest, transpose_and_slice_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({3, 5}));
  SizeType                       count = 0;
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 5; ++j)
    {
      t1.At(i, j) = TypeParam(count);
      ++count;
    }
  }
  fetch::math::Tensor<TypeParam> t2 = t1.Transpose();

  EXPECT_EQ(t2.shape(), std::vector<SizeType>({5, 3}));
  fetch::math::Tensor<TypeParam> t3 = t2.Slice(2).Copy();
  EXPECT_EQ(t3.shape(), std::vector<SizeType>({1, 3}));

  EXPECT_EQ(t3.At(0, 0), TypeParam(2));
  EXPECT_EQ(t3.At(0, 1), TypeParam(7));
  EXPECT_EQ(t3.At(0, 2), TypeParam(12));
}

TYPED_TEST(TensorOperationsTest, slice_and_transpose_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({3, 5, 2}));
  SizeType                       count = 0;
  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 5; ++k)
      {
        t1.At(j, k, i) = TypeParam(count);
        ++count;
      }
    }
  }

  fetch::math::Tensor<TypeParam> t2 = t1.Slice(1, 2).Copy();
  EXPECT_EQ(t2.shape(), std::vector<SizeType>({3, 5, 1}));

  EXPECT_EQ(t2.At(0, 0, 0), TypeParam(15));
  EXPECT_EQ(t2.At(1, 0, 0), TypeParam(20));
  EXPECT_EQ(t2.At(2, 0, 0), TypeParam(25));
  EXPECT_EQ(t2.At(0, 1, 0), TypeParam(16));
  EXPECT_EQ(t2.At(1, 1, 0), TypeParam(21));
  EXPECT_EQ(t2.At(2, 1, 0), TypeParam(26));
  EXPECT_EQ(t2.At(0, 2, 0), TypeParam(17));
  EXPECT_EQ(t2.At(1, 2, 0), TypeParam(22));
  EXPECT_EQ(t2.At(2, 2, 0), TypeParam(27));
  EXPECT_EQ(t2.At(0, 3, 0), TypeParam(18));
  EXPECT_EQ(t2.At(1, 3, 0), TypeParam(23));
  EXPECT_EQ(t2.At(2, 3, 0), TypeParam(28));
  EXPECT_EQ(t2.At(0, 4, 0), TypeParam(19));
  EXPECT_EQ(t2.At(1, 4, 0), TypeParam(24));
  EXPECT_EQ(t2.At(2, 4, 0), TypeParam(29));

  fetch::math::Tensor<TypeParam> t3 = (t2.Squeeze()).Transpose();
  EXPECT_EQ(t3.shape(), std::vector<SizeType>({5, 3}));

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

TYPED_TEST(TensorOperationsTest, multiple_slices_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({3, 5, 2}));
  SizeType                       count = 0;
  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 5; ++k)
      {
        t1.At(j, k, i) = TypeParam(count);
        ++count;
      }
    }
  }

  fetch::math::Tensor<TypeParam> t2 = t1.Slice(1, 2).Slice(2, 1).Copy();
  EXPECT_EQ(t2.shape(), std::vector<SizeType>({3, 1, 1}));

  EXPECT_EQ(t2.At(0, 0, 0), TypeParam(17));
  EXPECT_EQ(t2.At(1, 0, 0), TypeParam(22));
  EXPECT_EQ(t2.At(2, 0, 0), TypeParam(27));
}

TYPED_TEST(TensorOperationsTest, multiple_slices_separated_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({3, 5, 2}));
  SizeType                       count = 0;
  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 5; ++k)
      {
        t1.At(j, k, i) = TypeParam(count);
        ++count;
      }
    }
  }

  auto t1s = t1.Slice(1, 2);
  auto t2s = t1s.Slice(2, 1);

  fetch::math::Tensor<TypeParam> t1t = t1s.Copy();
  EXPECT_EQ(t1t.shape(), std::vector<SizeType>({3, 5, 1}));

  EXPECT_EQ(t1t.At(0, 0, 0), TypeParam(15));
  EXPECT_EQ(t1t.At(1, 0, 0), TypeParam(20));
  EXPECT_EQ(t1t.At(2, 0, 0), TypeParam(25));
  EXPECT_EQ(t1t.At(0, 1, 0), TypeParam(16));
  EXPECT_EQ(t1t.At(1, 1, 0), TypeParam(21));
  EXPECT_EQ(t1t.At(2, 1, 0), TypeParam(26));
  EXPECT_EQ(t1t.At(0, 2, 0), TypeParam(17));
  EXPECT_EQ(t1t.At(1, 2, 0), TypeParam(22));
  EXPECT_EQ(t1t.At(2, 2, 0), TypeParam(27));
  EXPECT_EQ(t1t.At(0, 3, 0), TypeParam(18));
  EXPECT_EQ(t1t.At(1, 3, 0), TypeParam(23));
  EXPECT_EQ(t1t.At(2, 3, 0), TypeParam(28));
  EXPECT_EQ(t1t.At(0, 4, 0), TypeParam(19));
  EXPECT_EQ(t1t.At(1, 4, 0), TypeParam(24));
  EXPECT_EQ(t1t.At(2, 4, 0), TypeParam(29));

  fetch::math::Tensor<TypeParam> t2t = t2s.Copy();
  EXPECT_EQ(t2t.shape(), std::vector<SizeType>({3, 1, 1}));

  EXPECT_EQ(t2t.At(0, 0, 0), TypeParam(17));
  EXPECT_EQ(t2t.At(1, 0, 0), TypeParam(22));
  EXPECT_EQ(t2t.At(2, 0, 0), TypeParam(27));
}

TYPED_TEST(TensorOperationsTest, multiple_const_slices_separated_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({3, 5, 2}));
  SizeType                       count = 0;
  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 5; ++k)
      {
        t1.At(j, k, i) = TypeParam(count);
        ++count;
      }
    }
  }

  const fetch::math::Tensor<TypeParam> t1c = t1.Copy();

  auto t1s = t1c.Slice(1, 2);
  auto t2s = t1s.Slice(2, 1);

  fetch::math::Tensor<TypeParam> t1t = t1s.Copy();
  EXPECT_EQ(t1t.shape(), std::vector<SizeType>({3, 5, 1}));

  EXPECT_EQ(t1t.At(0, 0, 0), TypeParam(15));
  EXPECT_EQ(t1t.At(1, 0, 0), TypeParam(20));
  EXPECT_EQ(t1t.At(2, 0, 0), TypeParam(25));
  EXPECT_EQ(t1t.At(0, 1, 0), TypeParam(16));
  EXPECT_EQ(t1t.At(1, 1, 0), TypeParam(21));
  EXPECT_EQ(t1t.At(2, 1, 0), TypeParam(26));
  EXPECT_EQ(t1t.At(0, 2, 0), TypeParam(17));
  EXPECT_EQ(t1t.At(1, 2, 0), TypeParam(22));
  EXPECT_EQ(t1t.At(2, 2, 0), TypeParam(27));
  EXPECT_EQ(t1t.At(0, 3, 0), TypeParam(18));
  EXPECT_EQ(t1t.At(1, 3, 0), TypeParam(23));
  EXPECT_EQ(t1t.At(2, 3, 0), TypeParam(28));
  EXPECT_EQ(t1t.At(0, 4, 0), TypeParam(19));
  EXPECT_EQ(t1t.At(1, 4, 0), TypeParam(24));
  EXPECT_EQ(t1t.At(2, 4, 0), TypeParam(29));

  fetch::math::Tensor<TypeParam> t2t = t2s.Copy();
  EXPECT_EQ(t2t.shape(), std::vector<SizeType>({3, 1, 1}));

  EXPECT_EQ(t2t.At(0, 0, 0), TypeParam(17));
  EXPECT_EQ(t2t.At(1, 0, 0), TypeParam(22));
  EXPECT_EQ(t2t.At(2, 0, 0), TypeParam(27));
}

TYPED_TEST(TensorOperationsTest, multiple_slices_assign_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({3, 5, 2}));
  fetch::math::Tensor<TypeParam> t2(std::vector<SizeType>({3, 2, 3}));

  SizeType count = 0;
  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 5; ++k)
      {
        t1.At(j, k, i) = TypeParam(count);
        ++count;
      }
    }
  }

  count = 0;
  for (SizeType i{0}; i < 3; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 2; ++k)
      {
        t2.At(j, k, i) = TypeParam(count);
        ++count;
      }
    }
  }

  const fetch::math::Tensor<TypeParam> t2c = t2.Copy();

  auto t3s = t1.Slice(1, 2).Slice(2, 1);
  auto t4s = t2c.Slice(1, 2).Slice(1, 1);

  t3s.Assign(t4s);

  fetch::math::Tensor<TypeParam> t3 = t3s.Copy();
  fetch::math::Tensor<TypeParam> t4 = t4s.Copy();

  EXPECT_EQ(t3.shape(), std::vector<SizeType>({3, 1, 1}));
  EXPECT_EQ(t4.shape(), std::vector<SizeType>({3, 1, 1}));

  EXPECT_EQ(t3.At(0, 0, 0), TypeParam(7));
  EXPECT_EQ(t3.At(1, 0, 0), TypeParam(9));
  EXPECT_EQ(t3.At(2, 0, 0), TypeParam(11));
}

TYPED_TEST(TensorOperationsTest, slices_same_tensor_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t1(std::vector<SizeType>({3, 5, 2}));

  SizeType count = 0;
  for (SizeType i{0}; i < 2; ++i)
  {
    for (SizeType j{0}; j < 3; ++j)
    {
      for (SizeType k{0}; k < 5; ++k)
      {
        t1.At(j, k, i) = static_cast<TypeParam>(count);
        ++count;
      }
    }
  }

  // Create first and second slice
  auto t1s = t1.Slice(1, 2).Slice(2, 1);
  auto t2s = t1.Slice(1, 2).Slice(2, 0);

  auto it = t1s.begin();

  // Modify first slice
  count = 0;
  while (it.is_valid())
  {
    *it = static_cast<TypeParam>(count);
    ++it;
    count++;
  }

  fetch::math::Tensor<TypeParam> t1t = t1s.Copy();
  fetch::math::Tensor<TypeParam> t2t = t2s.Copy();

  EXPECT_EQ(t1t.shape(), std::vector<SizeType>({3, 1, 1}));
  EXPECT_EQ(t2t.shape(), std::vector<SizeType>({1, 5, 1}));

  // Test second slice
  EXPECT_EQ(t2t.At(0, 0, 0), TypeParam(25));
  EXPECT_EQ(t2t.At(0, 1, 0), TypeParam(26));
  EXPECT_EQ(t2t.At(0, 2, 0), TypeParam(2));
  EXPECT_EQ(t2t.At(0, 3, 0), TypeParam(28));
  EXPECT_EQ(t2t.At(0, 4, 0), TypeParam(29));

  // Test original tensor
  EXPECT_EQ(t1.At(2, 0, 1), TypeParam(25));
  EXPECT_EQ(t1.At(2, 1, 1), TypeParam(26));
  EXPECT_EQ(t1.At(0, 2, 1), TypeParam(0));
  EXPECT_EQ(t1.At(1, 2, 1), TypeParam(1));
  EXPECT_EQ(t1.At(2, 2, 1), TypeParam(2));
  EXPECT_EQ(t1.At(2, 3, 1), TypeParam(28));
  EXPECT_EQ(t1.At(2, 4, 1), TypeParam(29));
}

// TODO (private 867) - reimplement shuffle & test
