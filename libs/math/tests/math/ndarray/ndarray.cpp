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
          temp_vector.push_back(a.Get({i, j, l, k}));
        }

        ASSERT_TRUE(b.Get({i, j, k}) == *std::max_element(temp_vector.begin(), temp_vector.end()));
        temp_vector.clear();
      }
    }
  }
}
