//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/matrix_operations.hpp"
#include "math/tensor/tensor.hpp"
#include "math/tensor/tensor_slice_iterator.hpp"

#include "gtest/gtest.h"

#include <cassert>
#include <iterator>
#include <vector>

using namespace fetch::math;

TEST(tensor_iterator, reshape_iterator_test)
{
  Tensor<double> a = Tensor<double>::Arange(0., 20., 1.);
  a.Reshape({1, a.size()});

  Tensor<double> b{a};
  b.Reshape({b.size(), 1});

  auto it1 = a.begin();
  auto it2 = b.begin();
  while (it1.is_valid())
  {
    EXPECT_EQ((*it1), (*it2));
    ++it1;
    ++it2;
  }
}

TEST(tensor_iterator, simple_iterator_permute_test)
{
  // set up an initial array
  Tensor<double> array{Tensor<double>::Arange(0., 77., 1.)};
  array.Reshape({7, 11});
  EXPECT_EQ(array.size(), 77);

  Tensor<double> ret;
  ret.Reshape(array.shape());

  EXPECT_EQ(ret.size(), array.size());
  EXPECT_EQ(ret.shape(), array.shape());
  TensorSliceIterator<double, Tensor<double>::ContainerType> it(array);
  TensorSliceIterator<double, Tensor<double>::ContainerType> it2(ret);

  it2.PermuteAxes(0, 1);
  while (it2)
  {
    ASSERT_TRUE(bool(it));
    ASSERT_TRUE(bool(it2));

    *it2 = *it;
    ++it;
    ++it2;
  }

  ASSERT_FALSE(bool(it));
  ASSERT_FALSE(bool(it2));

  SizeType test_val, cur_row;

  for (SizeType i = 0; i < array.size(); ++i)
  {
    EXPECT_EQ(array[i], double(i));

    cur_row  = i / 7;
    test_val = (11 * (i % 7)) + cur_row;

    EXPECT_EQ(double(ret[i]), double(test_val));
  }
}

TEST(tensor_iterator, iterator_4dim_copy_test)
{

  // set up an initial array
  Tensor<double> array{Tensor<double>::Arange(0., 1008., 1.)};
  array.Reshape({4, 6, 7, 6});
  Tensor<double> ret = array.Copy();

  TensorSliceIterator<double, Tensor<double>::ContainerType> it(
      array, {{1, 2, 1}, {2, 3, 1}, {1, 4, 1}, {2, 6, 1}});
  TensorSliceIterator<double, Tensor<double>::ContainerType> it2(
      ret, {{1, 2, 1}, {2, 3, 1}, {1, 4, 1}, {2, 6, 1}});

  while (it2)
  {
    assert(bool(it));
    assert(bool(it2));

    *it2 = *it;
    ++it;
    ++it2;
  }

  for (SizeType i = 0; i < 4; ++i)
  {
    for (SizeType j = 0; j < 6; ++j)
    {
      for (SizeType k = 0; k < 7; ++k)
      {
        for (SizeType l = 0; l < 6; ++l)
        {
          std::vector<SizeType> idxs = {i, j, k, l};
          EXPECT_EQ(int(ret.Get(idxs)), int(array.Get(idxs)));
        }
      }
    }
  }
}

TEST(Tensor, iterator_4dim_permute_test)
{
  // set up an initial array
  Tensor<double> array{Tensor<double>::Arange(0., 1008., 1.)};
  array.Reshape({4, 6, 7, 6});
  Tensor<double> ret = array.Copy();

  TensorSliceIterator<double, Tensor<double>::ContainerType> it(
      array, {{1, 2, 1}, {0, 6, 1}, {1, 4, 1}, {0, 6, 1}});
  TensorSliceIterator<double, Tensor<double>::ContainerType> it2(
      ret, {{1, 2, 1}, {0, 6, 1}, {1, 4, 1}, {0, 6, 1}});

  it.PermuteAxes(1, 3);
  while (it2)
  {
    assert(bool(it));
    assert(bool(it2));

    *it2 = *it;
    ++it;
    ++it2;
  }

  for (SizeType i = 1; i < 2; ++i)
  {
    for (SizeType j = 0; j < 6; ++j)
    {
      for (SizeType k = 1; k < 4; ++k)
      {
        for (SizeType l = 0; l < 6; ++l)
        {
          std::vector<SizeType> idxs  = {i, j, k, l};
          std::vector<SizeType> idxs2 = {i, l, k, j};
          EXPECT_EQ(int(ret.Get(idxs)), int(array.Get(idxs2)));
        }
      }
    }
  }
}

TEST(Tensor, simple_iterator_transpose_test)
{
  std::vector<SizeType> perm{2, 1, 0};
  std::vector<SizeType> original_shape{2, 3, 4};
  std::vector<SizeType> new_shape;
  new_shape.reserve(perm.size());
  for (SizeType i : perm)
  {
    new_shape.push_back(original_shape[i]);
  }
  SizeType arr_size = fetch::math::Product(original_shape);

  // set up an initial array
  Tensor<double> array = Tensor<double>::Arange(0., static_cast<double>(arr_size), 1.);
  array.Reshape(original_shape);

  Tensor<double> ret = Tensor<double>::Arange(0., static_cast<double>(arr_size), 1.);
  ret.Reshape(new_shape);

  Tensor<double> test_array{original_shape};

  ASSERT_TRUE(ret.size() == array.size());

  TensorSliceIterator<double, Tensor<double>::ContainerType> it_arr(array);
  TensorSliceIterator<double, Tensor<double>::ContainerType> it_ret(ret);

  it_ret.Transpose(perm);
  while (it_ret)
  {
    ASSERT_TRUE(bool(it_arr));
    ASSERT_TRUE(bool(it_ret));

    *it_arr = *it_ret;
    ++it_arr;
    ++it_ret;
  }

  for (SizeType i = 0; i < array.shape()[0]; ++i)
  {
    for (SizeType j = 0; j < array.shape()[1]; ++j)
    {
      for (SizeType k = 0; k < array.shape()[2]; ++k)
      {
        EXPECT_EQ(array(i, j, k), ret(k, j, i));
      }
    }
  }

  TensorSliceIterator<double, Tensor<double>::ContainerType> it_arr2(test_array);
  TensorSliceIterator<double, Tensor<double>::ContainerType> it_ret2(ret);
  it_ret2.Transpose(perm);

  while (it_ret2)
  {
    ASSERT_TRUE(bool(it_arr2));
    ASSERT_TRUE(bool(it_ret2));

    *it_arr2 = *it_ret2;
    ++it_arr2;
    ++it_ret2;
  }

  for (SizeType j = 0; j < array.size(); ++j)
  {
    EXPECT_EQ(array[j], test_array[j]);
  }
}
