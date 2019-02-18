////------------------------------------------------------------------------------
////
////   Copyright 2018-2019 Fetch.AI Limited
////
////   Licensed under the Apache License, Version 2.0 (the "License");
////   you may not use this file except in compliance with the License.
////   You may obtain a copy of the License at
////
////       http://www.apache.org/licenses/LICENSE-2.0
////
////   Unless required by applicable law or agreed to in writing, software
////   distributed under the License is distributed on an "AS IS" BASIS,
////   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
////   See the License for the specific language governing permissions and
////   limitations under the License.
////
////------------------------------------------------------------------------------
//
//#include <iomanip>
//#include <iostream>
//
////#include "core/random/lcg.hpp"
//#include <gtest/gtest.h>
//
//#include "math/ndarray.hpp"
//#include "math/ndarray_broadcast.hpp"
//#include "math/ndarray_view.hpp"
////#include "vectorise/threading/pool.hpp"
//
//using namespace fetch::math;
//
//TEST(ndarray, simple_broadcast_test)
//{
//  NDArray<double> a = NDArray<double>::Arange(0u, 20u, 1u);
//  a.Reshape({1, a.size()});
//  NDArray<double> b{a};
//  b.Reshape({b.size(), 1});
//
//  NDArray<double> ret;
//
//  ASSERT_TRUE(Broadcast([](double &x, double y) { return x + y; }, a, b, ret));
//
//  for (std::size_t i = 0; i < ret.shape(0); ++i)
//  {
//    for (std::size_t j = 0; j < ret.shape(1); ++j)
//    {
//      std::vector<std::size_t> idxs = {i, j};
//      ASSERT_TRUE(static_cast<std::size_t>(ret.Get(idxs)) == i + j);
//    }
//  }
//}
//
//TEST(ndarray, broadcast_3D_test)
//{
//  NDArray<double> a = NDArray<double>::Arange(0u, 21u, 1u);
//  ASSERT_TRUE(a.size() == 21);
//  a.Reshape({1, 3, 7});
//  NDArray<double> b = NDArray<double>::Arange(0u, 21u, 1u);
//  ASSERT_TRUE(b.size() == 21);
//  b.Reshape({7, 3, 1});
//
//  NDArray<double> ret;
//
//  ASSERT_TRUE(Broadcast([](double &x, double y) { return x + y; }, a, b, ret));
//
//  for (std::size_t i = 0; i < 7; ++i)
//  {
//    for (std::size_t j = 0; j < 3; ++j)
//    {
//      for (std::size_t k = 0; k < 7; ++k)
//      {
//        std::vector<std::size_t> idxs  = {i, j, k};
//        std::vector<std::size_t> idxs2 = {0, j, k};
//        std::vector<std::size_t> idxs3 = {i, j, 0};
//        if ((i == 0) && (k == 0))
//        {
//          ASSERT_TRUE(ret.Get(idxs) == a.Get(idxs) + b.Get(idxs));
//        }
//        else if ((i > 0) && (k == 0))
//        {
//          ASSERT_TRUE(ret.Get(idxs) == b.Get(idxs) + a.Get(idxs2));
//        }
//        else if ((i == 0) && (k > 0))
//        {
//          ASSERT_TRUE(ret.Get(idxs) == a.Get(idxs) + b.Get(idxs3));
//        }
//        else
//        {
//          ASSERT_TRUE(ret.Get(idxs) == a.Get(idxs2) + b.Get(idxs3));
//        }
//      }
//    }
//  }
//}
//
//TEST(ndarray, broadcast_shape_size_test)
//{
//
//  NDArray<double> a = NDArray<double>::Arange(0u, 90u, 1u);
//  a.Reshape({1, 3, 1, 6, 5});
//  NDArray<double> b = NDArray<double>::Arange(0u, 42u, 1u);
//  b.Reshape({7, 3, 2, 1, 1});
//
//  std::vector<std::size_t> ret_shape = {7, 3, 2, 6, 5};
//  std::vector<std::size_t> ref_shape;
//  ShapeFromBroadcast(a.shape(), b.shape(), ref_shape);
//  ASSERT_TRUE(ref_shape == ret_shape);
//  ASSERT_TRUE(NDArray<double>::SizeFromShape(ref_shape) ==
//              std::accumulate(std::begin(ret_shape), std::end(ret_shape), std::size_t(1),
//                              std::multiplies<>()));
//}
