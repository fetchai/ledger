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

// #include <algorithm>
#include <iomanip>
#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

#include "math/free_functions/free_functions.hpp"
//#include "math/kernels/concurrent_vm.hpp"
#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "math/rectangular_array.hpp"
//#include "vectorise/threading/pool.hpp"

// using namespace fetch::memory;
// using namespace fetch::threading;
// using namespace std::chrono;

using namespace fetch::math;

using data_type            = double;
using container_type       = fetch::memory::SharedArray<data_type>;
using vector_register_type = typename container_type::vector_register_type;
#define N 200

NDArray<data_type, container_type> RandomArray(std::size_t n, std::size_t m)
{
  static fetch::random::LinearCongruentialGenerator gen;
  NDArray<data_type, container_type>                array1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    array1[i] = data_type(gen.AsDouble());
  }
  return array1;
}

template <typename D>
using _S = fetch::memory::SharedArray<D>;

template <typename D>
using _A = NDArray<D, _S<D>>;

TEST(ndarray, simple_reshape)
{
  _A<double> a = NDArray<double>(49);
  a.Reshape({7, 7});
  ASSERT_TRUE(a.shape().size() == 2);
  ASSERT_TRUE(a.shape()[0] == 7);
  ASSERT_TRUE(a.shape()[1] == 7);

  a.Reshape({1, 49});
  ASSERT_TRUE(a.shape().size() == 2);
  ASSERT_TRUE(a.shape()[0] == 1);
  ASSERT_TRUE(a.shape()[1] == 49);
}

TEST(ndarray, faulty_reshape)
{
  _A<double> a = NDArray<double>(49);

  ASSERT_FALSE(a.CanReshape({2, 4}));
}

TEST(ndarray, max_axis_tests)
{
  // test parameters
  std::vector<std::size_t> orig_shape{7, 4, 6, 9};
  int                      axis      = 2;
  std::size_t              data_size = 7 * 4 * 6 * 9;

  // set up the original array and the return array
  std::vector<std::size_t> new_shape{orig_shape};
  new_shape.erase(new_shape.begin() + axis);
  _A<double> a{orig_shape};

  for (std::size_t i = 0; i < data_size; ++i)
  {
    a[i] = double(i);
  }
  _A<double> b{new_shape};
  Max<double, typename NDArray<double>::container_type>(a, std::size_t(axis), b);

  std::vector<double> temp_vector;
  for (std::size_t i = 0; i < new_shape[0]; ++i)
  {
    for (std::size_t j = 0; j < new_shape[1]; ++j)
    {
      for (std::size_t k = 0; k < new_shape[3]; ++k)
      {
        for (std::size_t l = 0; l < orig_shape[2]; ++l)
        {
          std::vector<std::size_t> const idxs = {i, j, l, k};
          temp_vector.push_back(a.Get(idxs));
        }
        std::vector<std::size_t> const idxs2 = {i, j, k};
        ASSERT_TRUE(b.Get(idxs2) == *std::max_element(temp_vector.begin(), temp_vector.end()));
        temp_vector.clear();
      }
    }
  }
}

TEST(ndarray, col_row_major_tets)
{
  // Nothing interesting happens in a 1D major order flip
  std::vector<std::size_t> shape{10};
  _A<double>               array1{shape};
  for (std::size_t i = 0; i < array1.size(); ++i)
  {
    array1[i] = i;
  }
  array1.MajorOrderFlip();
  for (std::size_t i = 0; i < array1.size(); ++i)
  {
    ASSERT_TRUE(array1[i] == i);
  }
  array1.MajorOrderFlip();
  for (std::size_t i = 0; i < array1.size(); ++i)
  {
    ASSERT_TRUE(array1[i] == i);
  }
  array1.MajorOrderFlip();
  for (std::size_t i = 0; i < array1.size(); ++i)
  {
    ASSERT_TRUE(array1[i] == i);
  }

  // major order is actually flipped for 2D and up - lets try a few flips
  shape = {3, 4, 7, 6};
  _A<double> array2{shape};
  for (std::size_t i = 0; i < array2.size(); ++i)
  {
    array2[i] = i;
  }

  array1.Resize(array2.size());
  array1.Reshape(array2.shape());
  array1.Copy(array2);

  array2.MajorOrderFlip();
  array2.MajorOrderFlip();
  ASSERT_TRUE(array1 == array2);
}

TEST(ndarray, concat_test)
{
  // A trivial concat
  std::vector<std::size_t> shape{10};
  _A<double>               array1{shape};
  array1.FillArange(0, 10);
  _A<double> array2{shape};
  array2.FillArange(0, 10);
  _A<double> ret_array{20};

  fetch::math::Concat(ret_array, {array1, array2});

  for (std::size_t j = 0; j < 10; ++j)
  {
    ASSERT_TRUE(array1[j] == ret_array[j]);
  }
  for (std::size_t j = 0; j < 10; ++j)
  {
    ASSERT_TRUE(array2[j] == ret_array[j + 10]);
  }

  // A more interesting concat
  shape = {2, 10};
  _A<double> array3{shape};
  array1.FillArange(0, 20);
  _A<double> array4{shape};
  array2.FillArange(0, 20);
  _A<double> ret_array2{40};

  fetch::math::Concat(ret_array2, {array3, array4}, 1);
  std::vector<std::size_t> new_shape{2, 20};
  ASSERT_TRUE(ret_array2.shape() == new_shape);
  for (std::size_t i = 0; i < 2; ++i)
  {
    for (std::size_t j = 0; j < 10; ++j)
    {
      std::vector<std::size_t> idx = {i, j};
      ASSERT_TRUE(array3.Get(idx) == ret_array2.Get(idx));
    }
  }
  for (std::size_t i = 0; i < 2; ++i)
  {
    for (std::size_t j = 0; j < 10; ++j)
    {
      std::vector<std::size_t> idx  = {i, j};
      std::vector<std::size_t> idx2 = {i, j + 10};
      ASSERT_TRUE(array4.Get(idx) == ret_array2.Get(idx));
    }
  }
}
