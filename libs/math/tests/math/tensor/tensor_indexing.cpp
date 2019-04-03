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

/*
using MyTypes = ::testing::Types<int, unsigned int, long, unsigned long, float, double>;
TYPED_TEST_CASE(TensorIndexingTest, MyTypes);

TYPED_TEST(TensorIndexingTest, empty_tensor_test)
{
  fetch::math::Tensor<TypeParam> t;

  ASSERT_EQ(t.size(), 0);
  ASSERT_EQ(t.capacity(), 0);

  // ASSERT_EQ(t.OffsetOfElement({0}), 0);
  // ASSERT_EQ(t.OffsetOfElement({1}), 0);
  // ASSERT_EQ(t.OffsetOfElement({2}), 0);

//  ASSERT_EQ(t.DimensionSize(0), 0);
//  ASSERT_EQ(t.DimensionSize(1), 0);
//  ASSERT_EQ(t.DimensionSize(2), 0);
//  ASSERT_EQ(t.DimensionSize(3), 0);
}

TYPED_TEST(TensorIndexingTest, one_dimentional_tensor_test)
{
  fetch::math::Tensor<TypeParam> t({5});

  ASSERT_EQ(t.size(), 5);
  ASSERT_EQ(t.capacity(), 8);

  ASSERT_EQ(t.OffsetOfElement({0}), 0);
  ASSERT_EQ(t.OffsetOfElement({1}), 1);
  ASSERT_EQ(t.OffsetOfElement({2}), 2);
  ASSERT_EQ(t.OffsetOfElement({3}), 3);
  ASSERT_EQ(t.OffsetOfElement({4}), 4);

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<std::uint64_t>({0u}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<std::uint64_t>({1u}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<std::uint64_t>({2u}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<std::uint64_t>({3u}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<std::uint64_t>({4u}));

//  ASSERT_EQ(t.DimensionSize(0), 1);
//  ASSERT_EQ(t.DimensionSize(1), 0);
//  ASSERT_EQ(t.DimensionSize(2), 0);
//  ASSERT_EQ(t.DimensionSize(3), 0);
}

TYPED_TEST(TensorIndexingTest, one_dimentional_tensor_with_stride_test)
{
  fetch::math::Tensor<TypeParam> t({5}, {2});

  ASSERT_EQ(t.size(), 5);
  ASSERT_EQ(t.capacity(), 16);

  ASSERT_EQ(t.OffsetOfElement({0}), 0);
  ASSERT_EQ(t.OffsetOfElement({1}), 2);
  ASSERT_EQ(t.OffsetOfElement({2}), 4);
  ASSERT_EQ(t.OffsetOfElement({3}), 6);
  ASSERT_EQ(t.OffsetOfElement({4}), 8);

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<std::uint64_t>({0u}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<std::uint64_t>({1u}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<std::uint64_t>({2u}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<std::uint64_t>({3u}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<std::uint64_t>({4u}));

//  ASSERT_EQ(t.DimensionSize(0), 2);
//  ASSERT_EQ(t.DimensionSize(1), 0);
//  ASSERT_EQ(t.DimensionSize(2), 0);
//  ASSERT_EQ(t.DimensionSize(3), 0);
}

TYPED_TEST(TensorIndexingTest, two_dimentional_tensor_test)
{
  fetch::math::Tensor<TypeParam> t({3, 5});

  ASSERT_EQ(t.size(), 15);
  ASSERT_EQ(t.capacity(), 24);

  ASSERT_EQ(t.OffsetOfElement({0, 0}), 0);
  ASSERT_EQ(t.OffsetOfElement({0, 1}), 1);
  ASSERT_EQ(t.OffsetOfElement({0, 2}), 2);
  ASSERT_EQ(t.OffsetOfElement({0, 3}), 3);
  ASSERT_EQ(t.OffsetOfElement({0, 4}), 4);

  ASSERT_EQ(t.OffsetOfElement({1, 0}), 8);
  ASSERT_EQ(t.OffsetOfElement({1, 1}), 9);
  ASSERT_EQ(t.OffsetOfElement({1, 2}), 10);
  ASSERT_EQ(t.OffsetOfElement({1, 3}), 11);
  ASSERT_EQ(t.OffsetOfElement({1, 4}), 12);

  ASSERT_EQ(t.OffsetOfElement({2, 0}), 16);
  ASSERT_EQ(t.OffsetOfElement({2, 1}), 17);
  ASSERT_EQ(t.OffsetOfElement({2, 2}), 18);
  ASSERT_EQ(t.OffsetOfElement({2, 3}), 19);
  ASSERT_EQ(t.OffsetOfElement({2, 4}), 20);

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<std::uint64_t>({0, 0}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<std::uint64_t>({0, 1}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<std::uint64_t>({0, 2}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<std::uint64_t>({0, 3}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<std::uint64_t>({0, 4}));

  ASSERT_EQ(t.IndicesOfElement(5), std::vector<std::uint64_t>({1, 0}));
  ASSERT_EQ(t.IndicesOfElement(6), std::vector<std::uint64_t>({1, 1}));
  ASSERT_EQ(t.IndicesOfElement(7), std::vector<std::uint64_t>({1, 2}));
  ASSERT_EQ(t.IndicesOfElement(8), std::vector<std::uint64_t>({1, 3}));
  ASSERT_EQ(t.IndicesOfElement(9), std::vector<std::uint64_t>({1, 4}));

  ASSERT_EQ(t.IndicesOfElement(10), std::vector<std::uint64_t>({2, 0}));
  ASSERT_EQ(t.IndicesOfElement(11), std::vector<std::uint64_t>({2, 1}));
  ASSERT_EQ(t.IndicesOfElement(12), std::vector<std::uint64_t>({2, 2}));
  ASSERT_EQ(t.IndicesOfElement(13), std::vector<std::uint64_t>({2, 3}));
  ASSERT_EQ(t.IndicesOfElement(14), std::vector<std::uint64_t>({2, 4}));

//  ASSERT_EQ(t.DimensionSize(0), 8);
//  ASSERT_EQ(t.DimensionSize(1), 1);
//  ASSERT_EQ(t.DimensionSize(2), 0);
//  ASSERT_EQ(t.DimensionSize(3), 0);

}

TYPED_TEST(TensorIndexingTest, two_dimentional_tensor_with_stride_test)
{
  fetch::math::Tensor<TypeParam> t({3, 5}, {2, 3});

  ASSERT_EQ(t.size(), 15);
  ASSERT_EQ(t.capacity(), 96);

  ASSERT_EQ(t.OffsetOfElement({0, 0}), 0);
  ASSERT_EQ(t.OffsetOfElement({0, 1}), 3);
  ASSERT_EQ(t.OffsetOfElement({0, 2}), 6);
  ASSERT_EQ(t.OffsetOfElement({0, 3}), 9);
  ASSERT_EQ(t.OffsetOfElement({0, 4}), 12);

  ASSERT_EQ(t.OffsetOfElement({1, 0}), 32);
  ASSERT_EQ(t.OffsetOfElement({1, 1}), 35);
  ASSERT_EQ(t.OffsetOfElement({1, 2}), 38);
  ASSERT_EQ(t.OffsetOfElement({1, 3}), 41);
  ASSERT_EQ(t.OffsetOfElement({1, 4}), 44);

  ASSERT_EQ(t.OffsetOfElement({2, 0}), 64);
  ASSERT_EQ(t.OffsetOfElement({2, 1}), 67);
  ASSERT_EQ(t.OffsetOfElement({2, 2}), 70);
  ASSERT_EQ(t.OffsetOfElement({2, 3}), 73);
  ASSERT_EQ(t.OffsetOfElement({2, 4}), 76);

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<std::uint64_t>({0, 0}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<std::uint64_t>({0, 1}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<std::uint64_t>({0, 2}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<std::uint64_t>({0, 3}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<std::uint64_t>({0, 4}));

  ASSERT_EQ(t.IndicesOfElement(5), std::vector<std::uint64_t>({1, 0}));
  ASSERT_EQ(t.IndicesOfElement(6), std::vector<std::uint64_t>({1, 1}));
  ASSERT_EQ(t.IndicesOfElement(7), std::vector<std::uint64_t>({1, 2}));
  ASSERT_EQ(t.IndicesOfElement(8), std::vector<std::uint64_t>({1, 3}));
  ASSERT_EQ(t.IndicesOfElement(9), std::vector<std::uint64_t>({1, 4}));

  ASSERT_EQ(t.IndicesOfElement(10), std::vector<std::uint64_t>({2, 0}));
  ASSERT_EQ(t.IndicesOfElement(11), std::vector<std::uint64_t>({2, 1}));
  ASSERT_EQ(t.IndicesOfElement(12), std::vector<std::uint64_t>({2, 2}));
  ASSERT_EQ(t.IndicesOfElement(13), std::vector<std::uint64_t>({2, 3}));
  ASSERT_EQ(t.IndicesOfElement(14), std::vector<std::uint64_t>({2, 4}));

//  ASSERT_EQ(t.DimensionSize(0), 32);
//  ASSERT_EQ(t.DimensionSize(1), 3);
//  ASSERT_EQ(t.DimensionSize(2), 0);
//  ASSERT_EQ(t.DimensionSize(3), 0);

}

TYPED_TEST(TensorIndexingTest, three_dimentional_tensor_test)
{
  fetch::math::Tensor<TypeParam> t({2, 3, 5});

  ASSERT_EQ(t.size(), 30);
  ASSERT_EQ(t.capacity(), 48);

  ASSERT_EQ(t.OffsetOfElement({0, 0, 0}), 0);
  ASSERT_EQ(t.OffsetOfElement({0, 0, 1}), 1);
  ASSERT_EQ(t.OffsetOfElement({0, 0, 2}), 2);
  ASSERT_EQ(t.OffsetOfElement({0, 0, 3}), 3);
  ASSERT_EQ(t.OffsetOfElement({0, 0, 4}), 4);

  ASSERT_EQ(t.OffsetOfElement({0, 1, 0}), 8);
  ASSERT_EQ(t.OffsetOfElement({0, 1, 1}), 9);
  ASSERT_EQ(t.OffsetOfElement({0, 1, 2}), 10);
  ASSERT_EQ(t.OffsetOfElement({0, 1, 3}), 11);
  ASSERT_EQ(t.OffsetOfElement({0, 1, 4}), 12);

  ASSERT_EQ(t.OffsetOfElement({0, 2, 0}), 16);
  ASSERT_EQ(t.OffsetOfElement({0, 2, 1}), 17);
  ASSERT_EQ(t.OffsetOfElement({0, 2, 2}), 18);
  ASSERT_EQ(t.OffsetOfElement({0, 2, 3}), 19);
  ASSERT_EQ(t.OffsetOfElement({0, 2, 4}), 20);

  ASSERT_EQ(t.OffsetOfElement({1, 0, 0}), 24);
  ASSERT_EQ(t.OffsetOfElement({1, 0, 1}), 25);
  ASSERT_EQ(t.OffsetOfElement({1, 0, 2}), 26);
  ASSERT_EQ(t.OffsetOfElement({1, 0, 3}), 27);
  ASSERT_EQ(t.OffsetOfElement({1, 0, 4}), 28);

  ASSERT_EQ(t.OffsetOfElement({1, 1, 0}), 32);
  ASSERT_EQ(t.OffsetOfElement({1, 1, 1}), 33);
  ASSERT_EQ(t.OffsetOfElement({1, 1, 2}), 34);
  ASSERT_EQ(t.OffsetOfElement({1, 1, 3}), 35);
  ASSERT_EQ(t.OffsetOfElement({1, 1, 4}), 36);

  ASSERT_EQ(t.OffsetOfElement({1, 2, 0}), 40);
  ASSERT_EQ(t.OffsetOfElement({1, 2, 1}), 41);
  ASSERT_EQ(t.OffsetOfElement({1, 2, 2}), 42);
  ASSERT_EQ(t.OffsetOfElement({1, 2, 3}), 43);
  ASSERT_EQ(t.OffsetOfElement({1, 2, 4}), 44);

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<std::uint64_t>({0, 0, 0}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<std::uint64_t>({0, 0, 1}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<std::uint64_t>({0, 0, 2}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<std::uint64_t>({0, 0, 3}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<std::uint64_t>({0, 0, 4}));

  ASSERT_EQ(t.IndicesOfElement(5), std::vector<std::uint64_t>({0, 1, 0}));
  ASSERT_EQ(t.IndicesOfElement(6), std::vector<std::uint64_t>({0, 1, 1}));
  ASSERT_EQ(t.IndicesOfElement(7), std::vector<std::uint64_t>({0, 1, 2}));
  ASSERT_EQ(t.IndicesOfElement(8), std::vector<std::uint64_t>({0, 1, 3}));
  ASSERT_EQ(t.IndicesOfElement(9), std::vector<std::uint64_t>({0, 1, 4}));

  ASSERT_EQ(t.IndicesOfElement(10), std::vector<std::uint64_t>({0, 2, 0}));
  ASSERT_EQ(t.IndicesOfElement(11), std::vector<std::uint64_t>({0, 2, 1}));
  ASSERT_EQ(t.IndicesOfElement(12), std::vector<std::uint64_t>({0, 2, 2}));
  ASSERT_EQ(t.IndicesOfElement(13), std::vector<std::uint64_t>({0, 2, 3}));
  ASSERT_EQ(t.IndicesOfElement(14), std::vector<std::uint64_t>({0, 2, 4}));

  ASSERT_EQ(t.IndicesOfElement(15), std::vector<std::uint64_t>({1, 0, 0}));
  ASSERT_EQ(t.IndicesOfElement(16), std::vector<std::uint64_t>({1, 0, 1}));
  ASSERT_EQ(t.IndicesOfElement(17), std::vector<std::uint64_t>({1, 0, 2}));
  ASSERT_EQ(t.IndicesOfElement(18), std::vector<std::uint64_t>({1, 0, 3}));
  ASSERT_EQ(t.IndicesOfElement(19), std::vector<std::uint64_t>({1, 0, 4}));

  ASSERT_EQ(t.IndicesOfElement(20), std::vector<std::uint64_t>({1, 1, 0}));
  ASSERT_EQ(t.IndicesOfElement(21), std::vector<std::uint64_t>({1, 1, 1}));
  ASSERT_EQ(t.IndicesOfElement(22), std::vector<std::uint64_t>({1, 1, 2}));
  ASSERT_EQ(t.IndicesOfElement(23), std::vector<std::uint64_t>({1, 1, 3}));
  ASSERT_EQ(t.IndicesOfElement(24), std::vector<std::uint64_t>({1, 1, 4}));

  ASSERT_EQ(t.IndicesOfElement(25), std::vector<std::uint64_t>({1, 2, 0}));
  ASSERT_EQ(t.IndicesOfElement(26), std::vector<std::uint64_t>({1, 2, 1}));
  ASSERT_EQ(t.IndicesOfElement(27), std::vector<std::uint64_t>({1, 2, 2}));
  ASSERT_EQ(t.IndicesOfElement(28), std::vector<std::uint64_t>({1, 2, 3}));
  ASSERT_EQ(t.IndicesOfElement(29), std::vector<std::uint64_t>({1, 2, 4}));

//  ASSERT_EQ(t.DimensionSize(0), 24);
//  ASSERT_EQ(t.DimensionSize(1), 8);
//  ASSERT_EQ(t.DimensionSize(2), 1);
//  ASSERT_EQ(t.DimensionSize(3), 0);

  TypeParam s(0);
  for (std::uint64_t i(0); i < 2; i++)
  {
    for (std::uint64_t j(0); j < 3; j++)
    {
      for (std::uint64_t k(0); k < 5; k++)
      {
        t.Set({i, j, k}, s);
        ASSERT_EQ(t.At({i, j, k}), s);
        s++;
      }
    }
  }

  std::vector<TypeParam> gt({0,  1,  2,  3,  4,  0, 0, 0, 5,  6,  7,  8,  9,  0, 0, 0,
                             10, 11, 12, 13, 14, 0, 0, 0, 15, 16, 17, 18, 19, 0, 0, 0,
                             20, 21, 22, 23, 24, 0, 0, 0, 25, 26, 27, 28, 29, 0, 0, 0});

  for(std::size_t i=0 ; i< gt.size(); ++i)
  {
    ASSERT_EQ(gt[i], t.data()[i]);
  }  

}


TYPED_TEST(TensorIndexingTest, zero_stride_tensor_test)
{
  fetch::math::Tensor<TypeParam> t({2, 3, 5}, {0, 0, 0}, {0, 0, 0});

  ASSERT_EQ(t.size(), 30);
  ASSERT_EQ(t.capacity(), 1);

  for (std::uint64_t i(0); i < 30; ++i)
  {
    ASSERT_EQ(t.OffsetOfElement(t.IndicesOfElement(i)), 0);
    ASSERT_EQ(t.At(i), 0);
  }

  t.Set({1, 1, 1}, TypeParam(42));  // Setting single element
  for (std::uint64_t i(0); i < 2; i++)
  {
    for (std::uint64_t j(0); j < 3; j++)
    {
      for (std::uint64_t k(0); k < 5; k++)
      {
        // All element are now that value since they all point at the same memory offset
        ASSERT_EQ(t.At({i, j, k}), TypeParam(42));
      }
    }
  }

}

TYPED_TEST(TensorIndexingTest, two_dimentional_tensor_slicing_test)
{

  fetch::math::Tensor<TypeParam> t({3, 5});
  for (std::uint64_t i(0); i < 3; ++i)
  {
    for (std::uint64_t j(0); j < 5; ++j)
    {
      t.Set({i, j}, TypeParam(i));
    }
  }
  fetch::math::Tensor<TypeParam> t0 = t.Slice(0);
  fetch::math::Tensor<TypeParam> t1 = t.Slice(1);
  fetch::math::Tensor<TypeParam> t2 = t.Slice(2);

  EXPECT_EQ(t0.shape(), std::vector<std::uint64_t>({5}));
  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({5}));
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5}));
//  EXPECT_EQ(t.Storage().use_count(), 5);  // t, t0, t1, t2, temporary return

  for (std::uint64_t j(0); j < 5; ++j)
  {
    EXPECT_EQ(t0.At(j), TypeParam(0));
    t0.At(j) = TypeParam(77 + j);
    EXPECT_EQ(t1.At(j), TypeParam(1));
    t1.At(j) = TypeParam(88 + j);
    EXPECT_EQ(t2.At(j), TypeParam(2));
    t2.At(j) = TypeParam(99 + j);
  }

  EXPECT_EQ(t.At(0), TypeParam(77));
  EXPECT_EQ(t.At(1), TypeParam(78));
  EXPECT_EQ(t.At(2), TypeParam(79));
  EXPECT_EQ(t.At(3), TypeParam(80));
  EXPECT_EQ(t.At(4), TypeParam(81));
  EXPECT_EQ(t.At(5), TypeParam(88));
  EXPECT_EQ(t.At(6), TypeParam(89));
  EXPECT_EQ(t.At(7), TypeParam(90));
  EXPECT_EQ(t.At(8), TypeParam(91));
  EXPECT_EQ(t.At(9), TypeParam(92));
  EXPECT_EQ(t.At(10), TypeParam(99));
  EXPECT_EQ(t.At(11), TypeParam(100));
  EXPECT_EQ(t.At(12), TypeParam(101));
  EXPECT_EQ(t.At(13), TypeParam(102));
  EXPECT_EQ(t.At(14), TypeParam(103));
}


TYPED_TEST(TensorIndexingTest, two_dimentional_tensor_with_stride_slicing_test)
{

  fetch::math::Tensor<TypeParam> t({3, 5}, {2, 3});
  for (std::uint64_t i(0); i < 3; ++i)
  {
    for (std::uint64_t j(0); j < 5; ++j)
    {
      t.Set({i, j}, TypeParam(i));
    }
  }
  fetch::math::Tensor<TypeParam> t0 = t.Slice(0);
  fetch::math::Tensor<TypeParam> t1 = t.Slice(1);
  fetch::math::Tensor<TypeParam> t2 = t.Slice(2);

  EXPECT_EQ(t0.shape(), std::vector<std::uint64_t>({5}));
  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({5}));
  EXPECT_EQ(t2.shape(), std::vector<std::uint64_t>({5}));
//  EXPECT_EQ(t.Storage().use_count(), 5);  // t, t0, t1, t2, temporary return

  for (std::uint64_t j(0); j < 5; ++j)
  {
    EXPECT_EQ(t0.At(j), TypeParam(0));
    t0.At(j) = TypeParam(77 + j);
    EXPECT_EQ(t1.At(j), TypeParam(1));
    t1.At(j) = TypeParam(88 + j);
    EXPECT_EQ(t2.At(j), TypeParam(2));
    t2.At(j) = TypeParam(99 + j);
  }

  EXPECT_EQ(t.At(0), TypeParam(77));
  EXPECT_EQ(t.At(1), TypeParam(78));
  EXPECT_EQ(t.At(2), TypeParam(79));
  EXPECT_EQ(t.At(3), TypeParam(80));
  EXPECT_EQ(t.At(4), TypeParam(81));
  EXPECT_EQ(t.At(5), TypeParam(88));
  EXPECT_EQ(t.At(6), TypeParam(89));
  EXPECT_EQ(t.At(7), TypeParam(90));
  EXPECT_EQ(t.At(8), TypeParam(91));
  EXPECT_EQ(t.At(9), TypeParam(92));
  EXPECT_EQ(t.At(10), TypeParam(99));
  EXPECT_EQ(t.At(11), TypeParam(100));
  EXPECT_EQ(t.At(12), TypeParam(101));
  EXPECT_EQ(t.At(13), TypeParam(102));
  EXPECT_EQ(t.At(14), TypeParam(103));

}

TYPED_TEST(TensorIndexingTest, three_dimentional_tensor_slicing_test)
{
  fetch::math::Tensor<TypeParam> t({2, 3, 5});

  TypeParam v(0);
  for (std::uint64_t i(0); i < 2; ++i)
  {
    for (std::uint64_t j(0); j < 3; ++j)
    {
      for (std::uint64_t k(0); k < 5; ++k)
      {
        t.Set(std::vector<std::uint64_t>({i, j, k}), v);
        v += TypeParam(1);
      }
    }
  }

  fetch::math::Tensor<TypeParam> t0 = t.Slice(0);
  fetch::math::Tensor<TypeParam> t1 = t.Slice(1);
  EXPECT_EQ(t0.shape(), std::vector<std::uint64_t>({3, 5}));
  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({3, 5}));

  for (std::uint64_t j(0); j < 3; ++j)
  {
    for (std::uint64_t k(0); k < 5; ++k)
    {
      EXPECT_EQ(t0.At(std::vector<std::uint64_t>({j, k})), TypeParam(j * 5 + k));
      EXPECT_EQ(t1.At(std::vector<std::uint64_t>({j, k})), TypeParam(j * 5 + k + 15));
    }
  }

}

TYPED_TEST(TensorIndexingTest, double_slicing_test)
{
  fetch::math::Tensor<TypeParam> t({2, 3, 5});

  TypeParam v(0);
  for (std::uint64_t i(0); i < 2; ++i)
  {
    for (std::uint64_t j(0); j < 3; ++j)
    {
      for (std::uint64_t k(0); k < 5; ++k)
      {
        t.Set(std::vector<std::uint64_t>({i, j, k}), v);
        v = v + TypeParam(1);
      }
    }
  }

  fetch::math::Tensor<TypeParam> t1 = t.Slice(1);
  EXPECT_EQ(t1.shape(), std::vector<std::uint64_t>({3, 5}));
  fetch::math::Tensor<TypeParam> t1_1 = t1.Slice(1);
  EXPECT_EQ(t1_1.shape(), std::vector<std::uint64_t>({5}));

  EXPECT_EQ(t1_1.At(0), TypeParam(20));
  EXPECT_EQ(t1_1.At(1), TypeParam(21));
  EXPECT_EQ(t1_1.At(2), TypeParam(22));
  EXPECT_EQ(t1_1.At(3), TypeParam(23));
  EXPECT_EQ(t1_1.At(4), TypeParam(24));

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
  for (std::uint64_t i(0); i < t.size(); ++i)
  {
    EXPECT_EQ(t.At(i), TypeParam(i));
  }

}

TYPED_TEST(TensorIndexingTest, range_based_iteration_1d_with_stride)
{

  fetch::math::Tensor<TypeParam> t({5}, {3});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }
  for (std::uint64_t i(0); i < 5; ++i)
  {
    EXPECT_EQ(t.At(i), TypeParam(i));
  }

}
TYPED_TEST(TensorIndexingTest, range_based_iteration_2d)
{
  fetch::math::Tensor<TypeParam> t({5, 2});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }
  for (std::uint64_t i(0); i < t.size(); ++i)
  {
    EXPECT_EQ(t.At(i), TypeParam(i));
  }
}

TYPED_TEST(TensorIndexingTest, range_based_iteration_2d_with_stride)
{
  fetch::math::Tensor<TypeParam> t({6, 3}, {3, 2});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }
  for (std::uint64_t i(0); i < 5; ++i)
  {
    EXPECT_EQ(t.At(i), TypeParam(i));
  }
}

TYPED_TEST(TensorIndexingTest, range_based_iteration_3d)
{
  fetch::math::Tensor<TypeParam> t({5, 2, 4});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }
  for (std::uint64_t i(0); i < t.size(); ++i)
  {
    EXPECT_EQ(t.At(i), TypeParam(i));
  }
}

TYPED_TEST(TensorIndexingTest, range_based_iteration_3d_with_stride)
{
  fetch::math::Tensor<TypeParam> t({5, 2, 4}, {3, 2, 1});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }
  for (std::uint64_t i(0); i < 5; ++i)
  {
    EXPECT_EQ(t.At(i), TypeParam(i));
  }
}

TYPED_TEST(TensorIndexingTest, range_based_iteration_4d)
{
  fetch::math::Tensor<TypeParam> t({5, 2, 4, 6});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }
  for (std::uint64_t i(0); i < t.size(); ++i)
  {
    EXPECT_EQ(t.At(i), TypeParam(i));
  }
}

TYPED_TEST(TensorIndexingTest, range_based_iteration_4d_with_stride)
{
  fetch::math::Tensor<TypeParam> t({6, 3, 4, 6}, {3, 2, 2, 3});
  TypeParam                      i(0);
  for (TypeParam &e : t)
  {
    e = i;
    i += TypeParam(1);
  }
  for (std::uint64_t i(0); i < 5; ++i)
  {
    EXPECT_EQ(t.At(i), TypeParam(i));
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
  EXPECT_EQ(t.capacity(), 8);

  EXPECT_EQ(t.OffsetOfElement({0, 0}), 0);
  EXPECT_EQ(t.OffsetOfElement({0, 1}), 1);
  EXPECT_EQ(t.OffsetOfElement({0, 2}), 2);
  EXPECT_EQ(t.OffsetOfElement({0, 3}), 3);
  EXPECT_EQ(t.OffsetOfElement({0, 4}), 4);

  EXPECT_EQ(t.IndicesOfElement(0), std::vector<std::uint64_t>({0u, 0u}));
  EXPECT_EQ(t.IndicesOfElement(1), std::vector<std::uint64_t>({0u, 1u}));
  EXPECT_EQ(t.IndicesOfElement(2), std::vector<std::uint64_t>({0u, 2u}));
  EXPECT_EQ(t.IndicesOfElement(3), std::vector<std::uint64_t>({0u, 3u}));
  EXPECT_EQ(t.IndicesOfElement(4), std::vector<std::uint64_t>({0u, 4u}));

//  EXPECT_EQ(t.DimensionSize(0), 5);
//  EXPECT_EQ(t.DimensionSize(1), 1);
//  EXPECT_EQ(t.DimensionSize(2), 0);
//  EXPECT_EQ(t.DimensionSize(3), 0);

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
  EXPECT_EQ(t.capacity(), 24);

  EXPECT_EQ(t.OffsetOfElement({0, 0, 0}), 0);
  EXPECT_EQ(t.OffsetOfElement({0, 0, 1}), 1);
  EXPECT_EQ(t.OffsetOfElement({0, 0, 2}), 2);
  EXPECT_EQ(t.OffsetOfElement({0, 0, 3}), 3);
  EXPECT_EQ(t.OffsetOfElement({0, 0, 4}), 4);

  EXPECT_EQ(t.OffsetOfElement({0, 1, 0}), 8);
  EXPECT_EQ(t.OffsetOfElement({0, 1, 1}), 9);
  EXPECT_EQ(t.OffsetOfElement({0, 1, 2}), 10);
  EXPECT_EQ(t.OffsetOfElement({0, 1, 3}), 11);
  EXPECT_EQ(t.OffsetOfElement({0, 1, 4}), 12);

  EXPECT_EQ(t.OffsetOfElement({0, 2, 0}), 16);
  EXPECT_EQ(t.OffsetOfElement({0, 2, 1}), 17);
  EXPECT_EQ(t.OffsetOfElement({0, 2, 2}), 18);
  EXPECT_EQ(t.OffsetOfElement({0, 2, 3}), 19);
  EXPECT_EQ(t.OffsetOfElement({0, 2, 4}), 20);

  EXPECT_EQ(t.IndicesOfElement(0), std::vector<std::uint64_t>({0, 0, 0}));
  EXPECT_EQ(t.IndicesOfElement(1), std::vector<std::uint64_t>({0, 0, 1}));
  EXPECT_EQ(t.IndicesOfElement(2), std::vector<std::uint64_t>({0, 0, 2}));
  EXPECT_EQ(t.IndicesOfElement(3), std::vector<std::uint64_t>({0, 0, 3}));
  EXPECT_EQ(t.IndicesOfElement(4), std::vector<std::uint64_t>({0, 0, 4}));

  EXPECT_EQ(t.IndicesOfElement(5), std::vector<std::uint64_t>({0, 1, 0}));
  EXPECT_EQ(t.IndicesOfElement(6), std::vector<std::uint64_t>({0, 1, 1}));
  EXPECT_EQ(t.IndicesOfElement(7), std::vector<std::uint64_t>({0, 1, 2}));
  EXPECT_EQ(t.IndicesOfElement(8), std::vector<std::uint64_t>({0, 1, 3}));
  EXPECT_EQ(t.IndicesOfElement(9), std::vector<std::uint64_t>({0, 1, 4}));

  EXPECT_EQ(t.IndicesOfElement(10), std::vector<std::uint64_t>({0, 2, 0}));
  EXPECT_EQ(t.IndicesOfElement(11), std::vector<std::uint64_t>({0, 2, 1}));
  EXPECT_EQ(t.IndicesOfElement(12), std::vector<std::uint64_t>({0, 2, 2}));
  EXPECT_EQ(t.IndicesOfElement(13), std::vector<std::uint64_t>({0, 2, 3}));
  EXPECT_EQ(t.IndicesOfElement(14), std::vector<std::uint64_t>({0, 2, 4}));

//  EXPECT_EQ(t.DimensionSize(0), 24);
//  EXPECT_EQ(t.DimensionSize(1), 8);
//  EXPECT_EQ(t.DimensionSize(2), 1);
//  EXPECT_EQ(t.DimensionSize(3), 0);

  i = 0;
  for (TypeParam &e : t)
  {
    EXPECT_EQ(e, i);
    i += TypeParam(1);
  }
}

TYPED_TEST(TensorIndexingTest, one_dimentional_squeeze_test)
{
  fetch::math::Tensor<TypeParam> t({5});
//  ASSERT_THROW(t.Squeeze(), std::runtime_error);
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

  ASSERT_EQ(t.OffsetOfElement({0}), 0);
  ASSERT_EQ(t.OffsetOfElement({1}), 1);
  ASSERT_EQ(t.OffsetOfElement({2}), 2);
  ASSERT_EQ(t.OffsetOfElement({3}), 3);
  ASSERT_EQ(t.OffsetOfElement({4}), 4);

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<std::uint64_t>({0u}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<std::uint64_t>({1u}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<std::uint64_t>({2u}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<std::uint64_t>({3u}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<std::uint64_t>({4u}));

//  ASSERT_EQ(t.DimensionSize(0), 1);
//  ASSERT_EQ(t.DimensionSize(1), 0);
//  ASSERT_EQ(t.DimensionSize(2), 0);
//  ASSERT_EQ(t.DimensionSize(3), 0);

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
  ASSERT_EQ(t.capacity(), 24);

  ASSERT_EQ(t.OffsetOfElement({0, 0}), 0);
  ASSERT_EQ(t.OffsetOfElement({0, 1}), 1);
  ASSERT_EQ(t.OffsetOfElement({0, 2}), 2);
  ASSERT_EQ(t.OffsetOfElement({0, 3}), 3);
  ASSERT_EQ(t.OffsetOfElement({0, 4}), 4);

  ASSERT_EQ(t.OffsetOfElement({1, 0}), 8);
  ASSERT_EQ(t.OffsetOfElement({1, 1}), 9);
  ASSERT_EQ(t.OffsetOfElement({1, 2}), 10);
  ASSERT_EQ(t.OffsetOfElement({1, 3}), 11);
  ASSERT_EQ(t.OffsetOfElement({1, 4}), 12);

  ASSERT_EQ(t.OffsetOfElement({2, 0}), 16);
  ASSERT_EQ(t.OffsetOfElement({2, 1}), 17);
  ASSERT_EQ(t.OffsetOfElement({2, 2}), 18);
  ASSERT_EQ(t.OffsetOfElement({2, 3}), 19);
  ASSERT_EQ(t.OffsetOfElement({2, 4}), 20);

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<std::uint64_t>({0, 0}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<std::uint64_t>({0, 1}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<std::uint64_t>({0, 2}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<std::uint64_t>({0, 3}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<std::uint64_t>({0, 4}));

  ASSERT_EQ(t.IndicesOfElement(5), std::vector<std::uint64_t>({1, 0}));
  ASSERT_EQ(t.IndicesOfElement(6), std::vector<std::uint64_t>({1, 1}));
  ASSERT_EQ(t.IndicesOfElement(7), std::vector<std::uint64_t>({1, 2}));
  ASSERT_EQ(t.IndicesOfElement(8), std::vector<std::uint64_t>({1, 3}));
  ASSERT_EQ(t.IndicesOfElement(9), std::vector<std::uint64_t>({1, 4}));

  ASSERT_EQ(t.IndicesOfElement(10), std::vector<std::uint64_t>({2, 0}));
  ASSERT_EQ(t.IndicesOfElement(11), std::vector<std::uint64_t>({2, 1}));
  ASSERT_EQ(t.IndicesOfElement(12), std::vector<std::uint64_t>({2, 2}));
  ASSERT_EQ(t.IndicesOfElement(13), std::vector<std::uint64_t>({2, 3}));
  ASSERT_EQ(t.IndicesOfElement(14), std::vector<std::uint64_t>({2, 4}));

//  ASSERT_EQ(t.DimensionSize(0), 8);
//  ASSERT_EQ(t.DimensionSize(1), 1);
//  ASSERT_EQ(t.DimensionSize(2), 0);
//  ASSERT_EQ(t.DimensionSize(3), 0);

  i = 0;
  for (TypeParam &e : t)
  {
    EXPECT_EQ(e, i);
    i += TypeParam(1);
  }
}
*/