#include <iomanip>
#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

//#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "math/ndarray_broadcast.hpp"
#include "math/ndarray_view.hpp"
//#include "vectorise/threading/pool.hpp"

using namespace fetch::math;


TEST(ndarray, simple_broadcast_test)
{
  NDArray<double> a = NDArray<double>::Arange(0, 20, 1);
  a.Reshape({1, a.size()});
  NDArray<double> b{a};
  b.Reshape({b.size(), 1});

  NDArray<double> ret = NDArray<double>::Zeros(a.size() * b.size());

  ASSERT_TRUE(Broadcast([](double const &x, double const&y) { return x + y; }, a, b, ret));

  for(std::size_t i = 0; i < ret.shape(0); ++i)
  {
    for(std::size_t j = 0; j < ret.shape(1); ++j)
    {
      ASSERT_TRUE(ret.Get({i, j}) == i + j);
    }
  }

}

TEST(ndarray, broadcast_3D_test)
{
  NDArray<double> a = NDArray<double>::Arange(0, 21, 1);
  a.Reshape({1, 3, 7});
  NDArray<double> b = NDArray<double>::Arange(0, 21, 1);
  b.Reshape({7, 3, 1});

  NDArray<double> ret = NDArray<double>::Zeros(a.size() * 7);

  ASSERT_TRUE(Broadcast([](double const &x, double const&y) { return x + y; }, a, b, ret));

  for(std::size_t i = 0; i < 7; ++i)
  {
    for(std::size_t j = 0; j < 3; ++j)
    {
      for(std::size_t k = 0; k < 7; ++k)
      {
          if ((i == 0) && (k == 0)) {ASSERT_TRUE(ret.Get({i, j, k}) == a.Get({i, j, k}) + b.Get({i, j, k}));}
          else if ((i > 0) && (k == 0)) {ASSERT_TRUE(ret.Get({i, j, k}) == b.Get({i, j, k}) + a.Get({0, j, k}));}
          else if ((i == 0) && (k > 0)) {ASSERT_TRUE(ret.Get({i, j, k}) == a.Get({i, j, k}) + b.Get({i, j, 0}));}
          else {ASSERT_TRUE(ret.Get({i, j, k}) == a.Get({0, j, k}) + b.Get({i, j, 0}));}

      }
    }
  }

}