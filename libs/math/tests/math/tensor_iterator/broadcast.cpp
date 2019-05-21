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

#include <gtest/gtest.h>
#include <iostream>

#include "math/tensor.hpp"
#include "math/tensor_broadcast.hpp"

using namespace fetch::math;

TEST(tensor_iterator, simple_broadcast_test)
{
  Tensor<double> a = Tensor<double>::Arange(0u, 20u, 1u);
  a.Reshape({1, a.size()});

  Tensor<double> b{a};
  b.Reshape({b.size(), 1});

  Tensor<double> ret;

  ASSERT_TRUE(Broadcast([](double &x, double y) { return x + y; }, a, b, ret));

  for (SizeType i = 0; i < ret.shape(0); ++i)
  {
    for (SizeType j = 0; j < ret.shape(1); ++j)
    {
      std::vector<SizeType> idxs = {i, j};
      ASSERT_TRUE(static_cast<SizeType>(ret.Get(idxs)) == i + j);
    }
  }
}

TEST(Tensor, broadcast_3D_test)
{
  Tensor<double> a = Tensor<double>::Arange(0u, 21u, 1u);
  ASSERT_TRUE(a.size() == 21);
  a.Reshape({1, 3, 7});
  Tensor<double> b = Tensor<double>::Arange(0u, 21u, 1u);
  ASSERT_TRUE(b.size() == 21);
  b.Reshape({7, 3, 1});

  Tensor<double> ret;

  ASSERT_TRUE(Broadcast([](double &x, double y) { return x + y; }, a, b, ret));

  for (SizeType i = 0; i < 7; ++i)
  {
    for (SizeType j = 0; j < 3; ++j)
    {
      for (SizeType k = 0; k < 7; ++k)
      {
        std::vector<SizeType> idxs  = {i, j, k};
        std::vector<SizeType> idxs2 = {0, j, k};
        std::vector<SizeType> idxs3 = {i, j, 0};
        if ((i == 0) && (k == 0))
        {
          ASSERT_TRUE(ret.Get(idxs) == a.Get(idxs) + b.Get(idxs));
        }
        else if ((i > 0) && (k == 0))
        {
          ASSERT_TRUE(ret.Get(idxs) == b.Get(idxs) + a.Get(idxs2));
        }
        else if ((i == 0) && (k > 0))
        {
          ASSERT_TRUE(ret.Get(idxs) == a.Get(idxs) + b.Get(idxs3));
        }
        else
        {
          ASSERT_TRUE(ret.Get(idxs) == a.Get(idxs2) + b.Get(idxs3));
        }
      }
    }
  }
}

TEST(tensor_iterator, broadcast_shape_size_test)
{

  Tensor<double> a = Tensor<double>::Arange(0u, 90u, 1u);
  a.Reshape({1, 3, 1, 6, 5});
  Tensor<double> b = Tensor<double>::Arange(0u, 42u, 1u);
  b.Reshape({7, 3, 2, 1, 1});

  std::vector<SizeType> ret_shape = {7, 3, 2, 6, 5};
  std::vector<SizeType> ref_shape;
  ShapeFromBroadcast(a.shape(), b.shape(), ref_shape);
  ASSERT_TRUE(ref_shape == ret_shape);
  ASSERT_TRUE(Tensor<double>::SizeFromShape(ref_shape) ==
              std::accumulate(std::begin(ret_shape), std::end(ret_shape), SizeType(1),
                              std::multiplies<>()));
}