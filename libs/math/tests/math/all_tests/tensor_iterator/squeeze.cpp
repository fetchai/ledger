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

#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>

#include "math/tensor.hpp"
#include "math/tensor_squeeze.hpp"

using namespace fetch::math;

TEST(tensor_iterator, tensor_reduce_test)
{
  Tensor<double> a = Tensor<double>::Arange(0u, 3u * 4u * 5u, 1u);
  a.Reshape({3, 4, 5});

  Tensor<double> ret;

  Reduce([](double const &x, double const &z) { return x + z; }, a, ret);
  SizeType m = 0;

  for (SizeType j = 0; j < ret.shape(1); ++j)
  {
    for (SizeType i = 0; i < ret.shape(0); ++i)
    {
      double ref = 0.0;
      for (SizeType k = 3 * m; k < (m + 1) * 3; ++k)
      {
        ref += double(k);
      }
      ++m;

      std::vector<SizeType> idxs = {i, j};
      ASSERT_TRUE(ret.Get(idxs) == ref);
    }
  }
}

TEST(tensor_iterator, dimension_reduction)
{
  Tensor<double> a = Tensor<double>::Arange(0u, 3u * 4u * 5u, 1u);
  a.Reshape({3, 4, 5});

  Tensor<double> ret;

  Reduce([](double const &x, double const &z) { return std::max(x, z); }, a, ret, 2);
  SizeType m = 0;
  for (SizeType j = 0; j < ret.shape(1); ++j)
  {
    for (SizeType i = 0; i < ret.shape(0); ++i)
    {
      SizeType offset = i + j * 3;
      double   ref    = 0.0;
      for (SizeType k = 0; k < 5; ++k)
      {
        SizeType v = offset + k * 3 * 4;
        ref        = std::max(ref, double(v));
        ++m;
      }
      std::vector<SizeType> idxs = {i, j};
      ASSERT_TRUE(ret.Get(idxs) == ref);
    }
  }
}