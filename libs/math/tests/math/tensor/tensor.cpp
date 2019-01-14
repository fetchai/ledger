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
class TensorTest : public ::testing::Test
{
};

using MyTypes =
    ::testing::Types<char, unsigned char, int, unsigned int, long, unsigned long, float, double>;
TYPED_TEST_CASE(TensorTest, MyTypes);

TYPED_TEST(TensorTest, empty_tensor_test)
{
  fetch::math::Tensor<TypeParam> t;

  ASSERT_EQ(t.NumberOfElements(), 0);
  ASSERT_EQ(t.Capacity(), 0);

  // ASSERT_EQ(t.OffsetOfElement({0}), 0);
  // ASSERT_EQ(t.OffsetOfElement({1}), 0);
  // ASSERT_EQ(t.OffsetOfElement({2}), 0);

  ASSERT_EQ(t.DimensionSize(0), 0);
  ASSERT_EQ(t.DimensionSize(1), 0);
  ASSERT_EQ(t.DimensionSize(2), 0);
  ASSERT_EQ(t.DimensionSize(3), 0);
}

TYPED_TEST(TensorTest, one_dimentional_tensor_test)
{
  fetch::math::Tensor<TypeParam> t({5});

  ASSERT_EQ(t.NumberOfElements(), 5);
  ASSERT_EQ(t.Capacity(), 8);

  ASSERT_EQ(t.OffsetOfElement({0}), 0);
  ASSERT_EQ(t.OffsetOfElement({1}), 1);
  ASSERT_EQ(t.OffsetOfElement({2}), 2);
  ASSERT_EQ(t.OffsetOfElement({3}), 3);
  ASSERT_EQ(t.OffsetOfElement({4}), 4);

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<size_t>({0u}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<size_t>({1u}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<size_t>({2u}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<size_t>({3u}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<size_t>({4u}));

  ASSERT_EQ(t.DimensionSize(0), 1);
  ASSERT_EQ(t.DimensionSize(1), 0);
  ASSERT_EQ(t.DimensionSize(2), 0);
  ASSERT_EQ(t.DimensionSize(3), 0);
}

TYPED_TEST(TensorTest, one_dimentional_tensor_with_stride_test)
{
  fetch::math::Tensor<TypeParam> t({5}, {2});

  ASSERT_EQ(t.NumberOfElements(), 5);
  ASSERT_EQ(t.Capacity(), 16);

  ASSERT_EQ(t.OffsetOfElement({0}), 0);
  ASSERT_EQ(t.OffsetOfElement({1}), 2);
  ASSERT_EQ(t.OffsetOfElement({2}), 4);
  ASSERT_EQ(t.OffsetOfElement({3}), 6);
  ASSERT_EQ(t.OffsetOfElement({4}), 8);

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<size_t>({0u}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<size_t>({1u}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<size_t>({2u}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<size_t>({3u}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<size_t>({4u}));

  ASSERT_EQ(t.DimensionSize(0), 2);
  ASSERT_EQ(t.DimensionSize(1), 0);
  ASSERT_EQ(t.DimensionSize(2), 0);
  ASSERT_EQ(t.DimensionSize(3), 0);
}

TYPED_TEST(TensorTest, two_dimentional_tensor_test)
{
  fetch::math::Tensor<TypeParam> t({3, 5});

  ASSERT_EQ(t.NumberOfElements(), 15);
  ASSERT_EQ(t.Capacity(), 24);

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

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<size_t>({0, 0}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<size_t>({0, 1}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<size_t>({0, 2}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<size_t>({0, 3}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<size_t>({0, 4}));

  ASSERT_EQ(t.IndicesOfElement(5), std::vector<size_t>({1, 0}));
  ASSERT_EQ(t.IndicesOfElement(6), std::vector<size_t>({1, 1}));
  ASSERT_EQ(t.IndicesOfElement(7), std::vector<size_t>({1, 2}));
  ASSERT_EQ(t.IndicesOfElement(8), std::vector<size_t>({1, 3}));
  ASSERT_EQ(t.IndicesOfElement(9), std::vector<size_t>({1, 4}));

  ASSERT_EQ(t.IndicesOfElement(10), std::vector<size_t>({2, 0}));
  ASSERT_EQ(t.IndicesOfElement(11), std::vector<size_t>({2, 1}));
  ASSERT_EQ(t.IndicesOfElement(12), std::vector<size_t>({2, 2}));
  ASSERT_EQ(t.IndicesOfElement(13), std::vector<size_t>({2, 3}));
  ASSERT_EQ(t.IndicesOfElement(14), std::vector<size_t>({2, 4}));

  ASSERT_EQ(t.DimensionSize(0), 8);
  ASSERT_EQ(t.DimensionSize(1), 1);
  ASSERT_EQ(t.DimensionSize(2), 0);
  ASSERT_EQ(t.DimensionSize(3), 0);
}

TYPED_TEST(TensorTest, two_dimentional_tensor_with_stride_test)
{
  fetch::math::Tensor<TypeParam> t({3, 5}, {2, 3});

  ASSERT_EQ(t.NumberOfElements(), 15);
  ASSERT_EQ(t.Capacity(), 96);

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

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<size_t>({0, 0}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<size_t>({0, 1}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<size_t>({0, 2}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<size_t>({0, 3}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<size_t>({0, 4}));

  ASSERT_EQ(t.IndicesOfElement(5), std::vector<size_t>({1, 0}));
  ASSERT_EQ(t.IndicesOfElement(6), std::vector<size_t>({1, 1}));
  ASSERT_EQ(t.IndicesOfElement(7), std::vector<size_t>({1, 2}));
  ASSERT_EQ(t.IndicesOfElement(8), std::vector<size_t>({1, 3}));
  ASSERT_EQ(t.IndicesOfElement(9), std::vector<size_t>({1, 4}));

  ASSERT_EQ(t.IndicesOfElement(10), std::vector<size_t>({2, 0}));
  ASSERT_EQ(t.IndicesOfElement(11), std::vector<size_t>({2, 1}));
  ASSERT_EQ(t.IndicesOfElement(12), std::vector<size_t>({2, 2}));
  ASSERT_EQ(t.IndicesOfElement(13), std::vector<size_t>({2, 3}));
  ASSERT_EQ(t.IndicesOfElement(14), std::vector<size_t>({2, 4}));

  ASSERT_EQ(t.DimensionSize(0), 32);
  ASSERT_EQ(t.DimensionSize(1), 3);
  ASSERT_EQ(t.DimensionSize(2), 0);
  ASSERT_EQ(t.DimensionSize(3), 0);
}

TYPED_TEST(TensorTest, three_dimentional_tensor_test)
{
  fetch::math::Tensor<TypeParam> t({2, 3, 5});

  ASSERT_EQ(t.NumberOfElements(), 30);
  ASSERT_EQ(t.Capacity(), 48);

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

  ASSERT_EQ(t.IndicesOfElement(0), std::vector<size_t>({0, 0, 0}));
  ASSERT_EQ(t.IndicesOfElement(1), std::vector<size_t>({0, 0, 1}));
  ASSERT_EQ(t.IndicesOfElement(2), std::vector<size_t>({0, 0, 2}));
  ASSERT_EQ(t.IndicesOfElement(3), std::vector<size_t>({0, 0, 3}));
  ASSERT_EQ(t.IndicesOfElement(4), std::vector<size_t>({0, 0, 4}));

  ASSERT_EQ(t.IndicesOfElement(5), std::vector<size_t>({0, 1, 0}));
  ASSERT_EQ(t.IndicesOfElement(6), std::vector<size_t>({0, 1, 1}));
  ASSERT_EQ(t.IndicesOfElement(7), std::vector<size_t>({0, 1, 2}));
  ASSERT_EQ(t.IndicesOfElement(8), std::vector<size_t>({0, 1, 3}));
  ASSERT_EQ(t.IndicesOfElement(9), std::vector<size_t>({0, 1, 4}));

  ASSERT_EQ(t.IndicesOfElement(10), std::vector<size_t>({0, 2, 0}));
  ASSERT_EQ(t.IndicesOfElement(11), std::vector<size_t>({0, 2, 1}));
  ASSERT_EQ(t.IndicesOfElement(12), std::vector<size_t>({0, 2, 2}));
  ASSERT_EQ(t.IndicesOfElement(13), std::vector<size_t>({0, 2, 3}));
  ASSERT_EQ(t.IndicesOfElement(14), std::vector<size_t>({0, 2, 4}));

  ASSERT_EQ(t.IndicesOfElement(15), std::vector<size_t>({1, 0, 0}));
  ASSERT_EQ(t.IndicesOfElement(16), std::vector<size_t>({1, 0, 1}));
  ASSERT_EQ(t.IndicesOfElement(17), std::vector<size_t>({1, 0, 2}));
  ASSERT_EQ(t.IndicesOfElement(18), std::vector<size_t>({1, 0, 3}));
  ASSERT_EQ(t.IndicesOfElement(19), std::vector<size_t>({1, 0, 4}));

  ASSERT_EQ(t.IndicesOfElement(20), std::vector<size_t>({1, 1, 0}));
  ASSERT_EQ(t.IndicesOfElement(21), std::vector<size_t>({1, 1, 1}));
  ASSERT_EQ(t.IndicesOfElement(22), std::vector<size_t>({1, 1, 2}));
  ASSERT_EQ(t.IndicesOfElement(23), std::vector<size_t>({1, 1, 3}));
  ASSERT_EQ(t.IndicesOfElement(24), std::vector<size_t>({1, 1, 4}));

  ASSERT_EQ(t.IndicesOfElement(25), std::vector<size_t>({1, 2, 0}));
  ASSERT_EQ(t.IndicesOfElement(26), std::vector<size_t>({1, 2, 1}));
  ASSERT_EQ(t.IndicesOfElement(27), std::vector<size_t>({1, 2, 2}));
  ASSERT_EQ(t.IndicesOfElement(28), std::vector<size_t>({1, 2, 3}));
  ASSERT_EQ(t.IndicesOfElement(29), std::vector<size_t>({1, 2, 4}));

  ASSERT_EQ(t.DimensionSize(0), 24);
  ASSERT_EQ(t.DimensionSize(1), 8);
  ASSERT_EQ(t.DimensionSize(2), 1);
  ASSERT_EQ(t.DimensionSize(3), 0);

  TypeParam s(0);
  for (size_t i(0); i < 2; i++)
  {
    for (size_t j(0); j < 3; j++)
    {
      for (size_t k(0); k < 5; k++)
      {
        t.Set({i, j, k}, s);
        ASSERT_EQ(t.Get({i, j, k}), s);
        s++;
      }
    }
  }

  std::vector<TypeParam> gt({0,  1,  2,  3,  4,  0, 0, 0, 5,  6,  7,  8,  9,  0, 0, 0,
                             10, 11, 12, 13, 14, 0, 0, 0, 15, 16, 17, 18, 19, 0, 0, 0,
                             20, 21, 22, 23, 24, 0, 0, 0, 25, 26, 27, 28, 29, 0, 0, 0});
  ASSERT_EQ(*(t.Storage()), gt);
}
