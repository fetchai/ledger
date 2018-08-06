#include <iomanip>
#include <iostream>

#include "math/kernels/sign.hpp"
#include "math/shape_less_array.hpp"
#include <gtest/gtest.h>

using namespace fetch::math;
using data_type      = double;
using container_type = fetch::memory::SharedArray<data_type>;

ShapeLessArray<data_type, container_type> RandomArray(std::size_t n, data_type adj)
{
  static fetch::random::LinearCongruentialGenerator gen;
  ShapeLessArray<data_type, container_type>         a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = data_type(gen.AsDouble()) + adj;
  }
  return a1;
}

TEST(ndarray, l2_basic)
{
  std::size_t                               n                = 10000;
  ShapeLessArray<data_type, container_type> test_array       = RandomArray(n, -0.5);
  double                                    test_loss        = 0;
  double                                    manual_test_loss = 0;

  // check that sign(0) = 0
  test_loss = test_array.L2Loss();

  for (std::size_t i = 0; i < n; ++i)
  {
    manual_test_loss += (test_array[i] * test_array[i]);
  }
  manual_test_loss /= 2;

  ASSERT_TRUE(manual_test_loss == test_loss);
}
