//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
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

#include <iomanip>
#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

//#include "math/linalg/matrix.hpp"
#include "math/free_functions/free_functions.hpp"
#include "math/ndarray.hpp"
#include "math/ndarray_iterator.hpp"
#include "math/ndarray_view.hpp"
//#include "vectorise/threading/pool.hpp"

using namespace fetch::math;

TEST(ndarray, simple_iterator_permute_test)
{

  // set up an initial array
  NDArray<double> array{NDArray<double>::Arange(0u, 77u, 1u)};
  array.Reshape({7, 11});

  NDArray<double> ret;
  ret.ResizeFromShape(array.shape());

  ASSERT_TRUE(ret.size() == array.size());
  ASSERT_TRUE(ret.shape() == array.shape());
  NDArrayIterator<double, NDArray<double>::container_type> it(array);
  NDArrayIterator<double, NDArray<double>::container_type> it2(ret);

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

  std::size_t test_val, cur_row;
  for (std::size_t i = 0; i < array.size(); ++i)
  {
    ASSERT_TRUE(array[i] == double(i));

    cur_row  = i / 7;
    test_val = (11 * (i % 7)) + cur_row;

    ASSERT_TRUE(double(ret[i]) == double(test_val));
  }
}

TEST(ndarray, iterator_4dim_copy_test)
{

  // set up an initial array
  NDArray<double> array{NDArray<double>::Arange(0u, 1008u, 1u)};
  array.Reshape({4, 6, 7, 6});
  NDArray<double> ret = array.Copy();

  NDArrayIterator<double, NDArray<double>::container_type> it(
      array, {{1, 2, 1}, {2, 3, 1}, {1, 4, 1}, {2, 6, 1}});
  NDArrayIterator<double, NDArray<double>::container_type> it2(
      ret, {{1, 2, 1}, {2, 3, 1}, {1, 4, 1}, {2, 6, 1}});

  while (it2)
  {

    assert(bool(it));
    assert(bool(it2));

    *it2 = *it;
    ++it;
    ++it2;
  }

  for (std::size_t i = 0; i < 4; ++i)
  {
    for (std::size_t j = 0; j < 6; ++j)
    {
      for (std::size_t k = 0; k < 7; ++k)
      {
        for (std::size_t l = 0; l < 6; ++l)
        {
          std::vector<std::size_t> idxs = {i, j, k, l};
          ASSERT_TRUE(int(ret.Get(idxs)) == int(array.Get(idxs)));
        }
      }
    }
  }
}

TEST(ndarray, iterator_4dim_permute_test)
{

  // set up an initial array
  NDArray<double> array{NDArray<double>::Arange(0u, 1008u, 1u)};
  array.Reshape({4, 6, 7, 6});
  NDArray<double> ret = array.Copy();

  NDArrayIterator<double, NDArray<double>::container_type> it(
      array, {{1, 2, 1}, {0, 6, 1}, {1, 4, 1}, {0, 6, 1}});
  NDArrayIterator<double, NDArray<double>::container_type> it2(
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

  for (std::size_t i = 1; i < 2; ++i)
  {
    for (std::size_t j = 0; j < 6; ++j)
    {
      for (std::size_t k = 1; k < 4; ++k)
      {
        for (std::size_t l = 0; l < 6; ++l)
        {
          std::vector<std::size_t> idxs  = {i, j, k, l};
          std::vector<std::size_t> idxs2 = {i, l, k, j};
          ASSERT_TRUE(int(ret.Get(idxs)) == int(array.Get(idxs2)));
        }
      }
    }
  }
}

TEST(ndarray, simple_iterator_transpose_test)
{
  std::vector<std::size_t> perm{2, 1, 0};
  std::vector<std::size_t> unperm{0, 1, 2};
  std::vector<std::size_t> original_shape{2, 3, 4};
  std::vector<std::size_t> new_shape;
  for (std::size_t i = 0; i < perm.size(); ++i)
  {
    new_shape.push_back(original_shape[perm[i]]);
  }
  std::size_t arr_size = fetch::math::Product(original_shape);

  // set up an initial array
  NDArray<double> array =
      NDArray<double>::Arange(static_cast<std::size_t>(0u), arr_size, static_cast<std::size_t>(1u));
  array.Reshape(original_shape);

  NDArray<double> ret =
      NDArray<double>::Arange(static_cast<std::size_t>(0u), arr_size, static_cast<std::size_t>(1u));
  ret.Reshape(original_shape);

  NDArray<double> test_array{original_shape};
  test_array.FillArange(static_cast<std::size_t>(0u), arr_size);

  ASSERT_TRUE(ret.size() == array.size());
  ASSERT_TRUE(ret.shape() == array.shape());
  NDArrayIterator<double, NDArray<double>::container_type> it_arr(array);
  NDArrayIterator<double, NDArray<double>::container_type> it_ret(ret);

  it_ret.Transpose(perm);
  while (it_ret)
  {
    ASSERT_TRUE(bool(it_arr));
    ASSERT_TRUE(bool(it_ret));

    *it_arr = *it_ret;
    ++it_arr;
    ++it_ret;
  }

  NDArrayIterator<double, NDArray<double>::container_type> it_arr2(array);
  NDArrayIterator<double, NDArray<double>::container_type> it_ret2(ret);
  it_ret2.Transpose(unperm);
  while (it_ret2)
  {
    ASSERT_TRUE(bool(it_arr2));
    ASSERT_TRUE(bool(it_ret2));

    *it_arr2 = *it_ret2;
    ++it_arr2;
    ++it_ret2;
  }
  for (std::size_t j = 0; j < array.size(); ++j)
  {
    ASSERT_TRUE(array[j] == test_array[j]);
  }
}