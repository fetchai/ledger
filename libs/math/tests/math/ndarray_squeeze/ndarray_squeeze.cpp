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

#include <gtest/gtest.h>

#include "math/ndarray.hpp"
#include "math/ndarray_squeeze.hpp"
#include "math/ndarray_view.hpp"

using namespace fetch::math;

TEST(ndarray, ndarray_reduce_test)
{
  NDArray<double> a = NDArray<double>::Arange(0, 3 * 4 * 5, 1);
  a.Reshape({3, 4, 5});

  NDArray<double> ret;

  Reduce([](double &x, double y) { return x + y; }, a, ret);
  std::size_t m = 0;
  for (std::size_t j = 0; j < ret.shape(1); ++j)
  {
    for (std::size_t i = 0; i < ret.shape(0); ++i)
    {
      double ref = 0.0;
      for (std::size_t k = 3 * m; k < (m + 1) * 3; ++k)
      {
        ref += double(k);
      }
      ++m;
      std::vector<std::size_t> idxs = {i, j};
      ASSERT_TRUE(ret.Get(idxs) == ref);
    }
  }
}

TEST(ndarray, dimension_reduction)
{
  NDArray<double> a = NDArray<double>::Arange(0, 3 * 4 * 5, 1);
  a.Reshape({3, 4, 5});

  NDArray<double> ret;

  Reduce([](double &x, double y) { return std::max(x, y); }, a, ret, 2);
  std::size_t m = 0;
  for (std::size_t j = 0; j < ret.shape(1); ++j)
  {
    for (std::size_t i = 0; i < ret.shape(0); ++i)
    {
      std::size_t offset = i + j * 3;
      double      ref    = 0.0;
      for (std::size_t k = 0; k < 5; ++k)
      {
        std::size_t v = offset + k * 3 * 4;
        ref           = std::max(ref, double(v));
        ++m;
      }
      std::vector<std::size_t> idxs = {i, j};
      ASSERT_TRUE(ret.Get(idxs) == ref);
    }
  }
}
