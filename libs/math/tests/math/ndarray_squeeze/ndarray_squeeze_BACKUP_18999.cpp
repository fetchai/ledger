#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>

#include "math/ndarray.hpp"
#include "math/ndarray_squeeze.hpp"
#include "math/ndarray_view.hpp"

using namespace fetch::math;

<<<<<<< HEAD
TEST(ndarray, dimension_trivial_reduction)
=======
TEST(ndarray, ndarray_reduce_test)
>>>>>>> cf0c1b1a2d6c91280aef37a89790189c8e16d5b1
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

      ASSERT_TRUE(ret.Get({i, j}) == ref);
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
      //      std::cout << ret.Get({i, j}) << " " << ref << std::endl;
      ASSERT_TRUE(ret.Get({i, j}) == ref);
    }
  }
}
