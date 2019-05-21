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
class TensorIndexingTest : public ::testing::Test
{
};

using MyTypes = ::testing::Types<int, unsigned int, long, unsigned long, float, double>;
TYPED_TEST_CASE(TensorIndexingTest, MyTypes);

TYPED_TEST(TensorIndexingTest, empty_tensor_test)
{
  fetch::math::Tensor<TypeParam> t;
  ASSERT_EQ(t.size(), 0);
  ASSERT_EQ(t.shape().size(), 1);
}

TYPED_TEST(TensorIndexingTest, one_dimentional_tensor_test)
{
  fetch::math::Tensor<TypeParam> t({5});

  ASSERT_EQ(t.size(), 5);
  ASSERT_EQ(t.shape().size(), 1);
  ASSERT_EQ(t.shape()[0], 5);
}

TYPED_TEST(TensorIndexingTest, two_dimentional_tensor_test)
{
  fetch::math::Tensor<TypeParam> t({3, 5});

  ASSERT_EQ(t.size(), 15);
  ASSERT_EQ(t.shape().size(), 2);
  ASSERT_EQ(t.shape()[0], 3);
  ASSERT_EQ(t.shape()[1], 5);
}

TYPED_TEST(TensorIndexingTest, index_op_vs_iterator)
{
  TypeParam                      from      = TypeParam(20);
  TypeParam                      to        = TypeParam(29);
  TypeParam                      step_size = TypeParam(1);
  fetch::math::Tensor<TypeParam> a = fetch::math::Tensor<TypeParam>::Arange(from, to, step_size);
  EXPECT_EQ(a.size(), 9);
  a.Reshape({3, 3});

  fetch::math::Tensor<TypeParam> b{a.shape()};
  fetch::math::Tensor<TypeParam> c;
  c.Resize(a.shape());

  auto it1 = a.begin();
  auto it2 = b.begin();
  while (it1.is_valid())
  {
    *it2 = *it1;
    ++it1;
    ++it2;
  }

  for (std::size_t i = 0; i < a.size(); ++i)
  {
    c[i] = a[i];
  }

  EXPECT_EQ(a, c);
  EXPECT_EQ(b, c);
  EXPECT_EQ(b, a);
}

TYPED_TEST(TensorIndexingTest, three_dimentional_tensor_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t({2, 3, 5});

  ASSERT_EQ(t.size(), 30);

  ASSERT_EQ(t.shape().size(), 3);
  ASSERT_EQ(t.shape()[0], 2);
  ASSERT_EQ(t.shape()[1], 3);
  ASSERT_EQ(t.shape()[2], 5);

  TypeParam s(0);
  for (SizeType i{0}; i < 2; i++)
  {
    for (SizeType j(0); j < 3; j++)
    {
      for (SizeType k(0); k < 5; k++)
      {
        t.Set(i, j, k, s);
        ASSERT_EQ(t.At(i, j, k), s);
        s++;
      }
    }
  }

  std::vector<TypeParam> gt({0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
                             15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29});

  std::uint64_t counter = 0;
  for (std::uint64_t i(0); i < 2; i++)
  {
    for (std::uint64_t j(0); j < 3; j++)
    {
      for (std::uint64_t k(0); k < 5; k++)
      {
        ASSERT_EQ(gt[counter], t.At(i, j, k));
        ++counter;
      }
    }
  }
}

TYPED_TEST(TensorIndexingTest, double_slicing_test)
{
  using SizeType = typename fetch::math::Tensor<TypeParam>::SizeType;

  fetch::math::Tensor<TypeParam> t({2, 3, 5});

  TypeParam v(0);
  for (SizeType i(0); i < 2; ++i)
  {
    for (SizeType j(0); j < 3; ++j)
    {
      for (SizeType k(0); k < 5; ++k)
      {
        t.Set(i, j, k, v);
        v = v + TypeParam(1);
      }
    }
  }

  fetch::math::Tensor<TypeParam> t1 = t.Slice(1).Copy();
  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({1, 3, 5}));
  fetch::math::Tensor<TypeParam> t1_1 = t1.Slice(1, 1).Copy();
  EXPECT_EQ(t1_1.shape(), std::vector<std::uint64_t>({1, 1, 5}));

  EXPECT_EQ(t1_1.At(0, 0, 0), TypeParam(20));
  EXPECT_EQ(t1_1.At(0, 0, 1), TypeParam(21));
  EXPECT_EQ(t1_1.At(0, 0, 2), TypeParam(22));
  EXPECT_EQ(t1_1.At(0, 0, 3), TypeParam(23));
  EXPECT_EQ(t1_1.At(0, 0, 4), TypeParam(24));
}

TYPED_TEST(TensorIndexingTest, range_based_iteration_1d)
{

  fetch::math::Tensor<TypeParam> t({5});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }
  for (std::uint64_t k(0); k < t.size(); ++k)
  {
    EXPECT_EQ(t.At(k), TypeParam(k));
  }
}

TYPED_TEST(TensorIndexingTest, range_based_iteration_2d)
{
  fetch::math::Tensor<TypeParam> t({5, 2});
  TypeParam                      z(0);
  for (TypeParam &e : t)
  {
    e = z;
    z += TypeParam(1);
  }
  TypeParam val{0};
  for (std::uint64_t i(0); i < t.shape()[1]; ++i)
  {
    for (std::uint64_t j(0); j < t.shape()[0]; ++j)
    {
      EXPECT_EQ(t.At(j, i), val);
      ++val;
    }
  }
}

TYPED_TEST(TensorIndexingTest, range_based_iteration_3d)
{
  fetch::math::Tensor<TypeParam> t({5, 2, 4});
  TypeParam                      z(0);
  for (TypeParam &e : t)
  {
    e = z;
    z += TypeParam(1);
  }
  TypeParam val{0};
  for (std::uint64_t i(0); i < t.shape()[2]; ++i)
  {
    for (std::uint64_t j(0); j < t.shape()[1]; ++j)
    {
      for (std::uint64_t k(0); k < t.shape()[0]; ++k)
      {
        EXPECT_EQ(t.At(k, j, i), val);
        ++val;
      }
    }
  }
}

TYPED_TEST(TensorIndexingTest, range_based_iteration_4d)
{
  fetch::math::Tensor<TypeParam> t({5, 2, 4, 6});
  TypeParam                      z(0);
  for (TypeParam &e : t)
  {
    e = z;
    z += TypeParam(1);
  }
  TypeParam val{0};
  for (std::uint64_t i(0); i < t.shape()[3]; ++i)
  {
    for (std::uint64_t j(0); j < t.shape()[2]; ++j)
    {
      for (std::uint64_t k(0); k < t.shape()[1]; ++k)
      {
        for (std::uint64_t m(0); m < t.shape()[0]; ++m)
        {
          EXPECT_EQ(t.At(m, k, j, i), val);
          ++val;
        }
      }
    }
  }
}

TYPED_TEST(TensorIndexingTest, one_dimensional_unsqueeze_test)
{
  fetch::math::Tensor<TypeParam> t({5});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }

  EXPECT_EQ(t.shape(), std::vector<std::uint64_t>({5}));
  t.Unsqueeze();
  EXPECT_EQ(t.shape(), std::vector<std::uint64_t>({1, 5}));

  EXPECT_EQ(t.size(), 5);

  i = 0;
  for (TypeParam &e : t)
  {
    EXPECT_EQ(e, i);
    i += TypeParam(1);
  }
}

TYPED_TEST(TensorIndexingTest, two_dimentional_unsqueeze_test)
{
  fetch::math::Tensor<TypeParam> t({3, 5});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }

  EXPECT_EQ(t.shape(), std::vector<std::uint64_t>({3u, 5u}));
  t.Unsqueeze();
  EXPECT_EQ(t.shape(), std::vector<std::uint64_t>({1u, 3u, 5u}));

  EXPECT_EQ(t.size(), 15);

  i = 0;
  for (TypeParam &e : t)
  {
    EXPECT_EQ(e, i);
    i += TypeParam(1);
  }
}

TYPED_TEST(TensorIndexingTest, two_dimentional_squeeze_test)
{
  fetch::math::Tensor<TypeParam> t({1, 5});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }

  EXPECT_EQ(t.shape(), std::vector<std::uint64_t>({1u, 5u}));
  t.Squeeze();
  EXPECT_EQ(t.shape(), std::vector<std::uint64_t>({5u}));

  i = 0;
  for (TypeParam &e : t)
  {
    EXPECT_EQ(e, i);
    i += TypeParam(1);
  }
}

TYPED_TEST(TensorIndexingTest, three_dimentional_squeeze_test)
{
  fetch::math::Tensor<TypeParam> t({1, 3, 5});

  TypeParam i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }

  EXPECT_EQ(t.shape(), std::vector<std::uint64_t>({1u, 3u, 5u}));
  t.Squeeze();
  EXPECT_EQ(t.shape(), std::vector<std::uint64_t>({3u, 5u}));

  ASSERT_EQ(t.size(), 15);

  i = 0;
  for (TypeParam &e : t)
  {
    EXPECT_EQ(e, i);
    i += TypeParam(1);
  }
}

TYPED_TEST(TensorIndexingTest, major_order_flip_test)
{
  fetch::math::Tensor<TypeParam> t({3, 3});
  t.FillArange(static_cast<TypeParam>(0), static_cast<TypeParam>(t.size()));

  EXPECT_EQ(t[0], 0);
  EXPECT_EQ(t[1], 1);
  EXPECT_EQ(t[2], 2);
  EXPECT_EQ(t[3], 3);
  EXPECT_EQ(t[4], 4);
  EXPECT_EQ(t[5], 5);
  EXPECT_EQ(t[6], 6);
  EXPECT_EQ(t[7], 7);
  EXPECT_EQ(t[8], 8);

  t.MajorOrderFlip();

  EXPECT_EQ(t[0], 0);
  EXPECT_EQ(t[1], 3);
  EXPECT_EQ(t[2], 6);
  EXPECT_EQ(t[3], 1);
  EXPECT_EQ(t[4], 4);
  EXPECT_EQ(t[5], 7);
  EXPECT_EQ(t[6], 2);
  EXPECT_EQ(t[7], 5);
  EXPECT_EQ(t[8], 8);

  t.MajorOrderFlip();

  EXPECT_EQ(t[0], 0);
  EXPECT_EQ(t[1], 1);
  EXPECT_EQ(t[2], 2);
  EXPECT_EQ(t[3], 3);
  EXPECT_EQ(t[4], 4);
  EXPECT_EQ(t[5], 5);
  EXPECT_EQ(t[6], 6);
  EXPECT_EQ(t[7], 7);
  EXPECT_EQ(t[8], 8);
}
