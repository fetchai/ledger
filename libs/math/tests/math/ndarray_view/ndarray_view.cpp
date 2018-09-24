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

#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "math/ndarray_view.hpp"
#include "vectorise/threading/pool.hpp"

// using namespace fetch::memory;
// using namespace fetch::threading;
// using namespace std::chrono;

using namespace fetch::math;

using data_type            = double;
using container_type       = fetch::memory::SharedArray<data_type>;
using vector_register_type = typename container_type::vector_register_type;
#define N 200

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _A = NDArray<D, _S<D>>;

TEST(ndarray, 2d_view_full)
{
  // set up a 4d array
  std::vector<std::size_t> shape      = {3, 3};
  _A<double>               test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (3 * 3); ++i)
  {
    test_array[i] = double(i);
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 3, 1}, {0, 3, 1}};
  NDArrayView                           array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {3, 3};
  _A<double>               new_array = _A<double>(new_shape);
  new_array                          = test_array.GetRange(array_view);

  std::vector<std::size_t> compare_vals = {0, 1, 2, 3, 4, 5, 6, 7, 8};
  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
    ASSERT_TRUE(compare_vals[i] == new_array[i]);
  }
}

TEST(ndarray, 3d_view_full)
{
  // set up a 4d array
  std::vector<std::size_t> shape      = {5, 5, 5};
  _A<double>               test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5 * 5 * 5); ++i)
  {
    test_array[i] = double(i);
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 5, 1}, {0, 5, 1}, {0, 5, 1}};
  NDArrayView                           array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 5, 5};
  _A<double>               new_array = _A<double>(new_shape);
  new_array                          = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }
}

TEST(ndarray, 4d_view_full)
{
  // set up a 4d array
  std::vector<std::size_t> shape      = {5, 5, 5, 5};
  _A<double>               test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5 * 5 * 5 * 5); ++i)
  {
    test_array[i] = double(i);
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 5, 1}, {0, 5, 1}, {0, 5, 1}, {0, 5, 1}};
  NDArrayView                           array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 5, 5, 5};
  _A<double>               new_array = _A<double>(new_shape);
  new_array                          = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }
}

TEST(ndarray, 6d_view_full)
{
  // set up a 4d array
  std::vector<std::size_t> shape      = {5, 5, 5, 5, 5, 5};
  _A<double>               test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5 * 5 * 5 * 5 * 5 * 5); ++i)
  {
    test_array[i] = double(i);
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 5, 1}, {0, 5, 1}, {0, 5, 1},
                                                      {0, 5, 1}, {0, 5, 1}, {0, 5, 1}};
  NDArrayView                           array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 5, 5, 5, 5, 5};
  _A<double>               new_array = _A<double>(new_shape);
  new_array                          = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }
}

TEST(ndarray, 2d_irregular_view)
{
  // set up a 4d array
  std::vector<std::size_t> shape      = {5, 10};
  _A<double>               test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5 * 10); ++i)
  {
    test_array[i] = double(i);
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 5, 1}, {0, 10, 1}};
  NDArrayView                           array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 10};
  _A<double>               new_array = _A<double>(new_shape);
  new_array                          = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }
}

TEST(ndarray, 3d_irregular_view)
{
  // set up a 4d array
  std::vector<std::size_t> shape      = {5, 10, 10};
  _A<double>               test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (5 * 10 * 10); ++i)
  {
    test_array[i] = double(i);
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 5, 1}, {0, 10, 1}, {0, 10, 1}};
  NDArrayView                           array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {5, 10, 10};
  _A<double>               new_array = _A<double>(new_shape);
  new_array                          = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }
}

TEST(ndarray, 6d_irregular_view)
{
  // set up a 4d array
  std::vector<std::size_t> shape      = {1, 2, 3, 4, 5, 6};
  _A<double>               test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (1 * 2 * 3 * 4 * 5 * 6); ++i)
  {
    test_array[i] = double(i);
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 1, 1}, {0, 2, 1}, {0, 3, 1},
                                                      {0, 4, 1}, {0, 5, 1}, {0, 6, 1}};
  NDArrayView                           array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {1, 2, 3, 4, 5, 6};
  _A<double>               new_array = _A<double>(new_shape);
  new_array                          = test_array.GetRange(array_view);

  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(test_array[i] == new_array[i]);
  }
}

TEST(ndarray, 2d_big_step)
{
  std::size_t step = 2;

  // set up a 4d array
  std::vector<std::size_t> shape      = {4, 4};
  _A<double>               test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (4 * 4); ++i)
  {
    test_array[i] = double(i);
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 4, step}, {0, 4, step}};
  NDArrayView                           array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {4, 4};
  _A<double>               new_array = _A<double>(new_shape);
  new_array                          = test_array.GetRange(array_view);

  std::vector<std::size_t> true_output{0, 2, 8, 10};
  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(true_output[i] == new_array[i]);
  }
}

TEST(ndarray, 2d_subset_view)
{
  // set up a 4d array
  std::vector<std::size_t> shape      = {4, 4};
  _A<double>               test_array = _A<double>(shape);
  for (std::size_t i = 0; i < (4 * 4); ++i)
  {
    test_array[i] = double(i);
  }

  // set up a valid view shape
  std::vector<std::vector<std::size_t>> view_shape = {{0, 2, 1}, {0, 2, 1}};
  NDArrayView                           array_view = NDArrayView();
  for (auto item : view_shape)
  {
    array_view.from.push_back(item[0]);
    array_view.to.push_back(item[1]);
    array_view.step.push_back(item[2]);
  }

  // extract the view shape into a new ndarray
  std::vector<std::size_t> new_shape = {3, 3, 3};
  _A<double>               new_array = _A<double>(new_shape);
  new_array                          = test_array.GetRange(array_view);

  std::vector<std::size_t> true_output{0, 1, 4, 5};
  for (std::size_t i = 0; i < new_array.data().size(); ++i)
  {
    ASSERT_TRUE(true_output[i] == new_array[i]);
  }
}
