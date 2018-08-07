#include <iomanip>
#include <iostream>

#include "math/kernels/L2Norm.hpp"
#include "math/shape_less_array.hpp"
#include <gtest/gtest.h>

using namespace fetch::math;
using data_type      = double;
using container_type = fetch::memory::SharedArray<data_type>;

ShapeLessArray<data_type, container_type> RandomArray(std::size_t n)
{
  static fetch::random::LinearCongruentialGenerator gen;
  ShapeLessArray<data_type, container_type>         a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = data_type(gen.AsDouble());
  }
  return a1;
}

TEST(ndarray, zeros_out)
{
  std::size_t                               n            = 1000;
  ShapeLessArray<data_type, container_type> test_array   = RandomArray(n);
  ShapeLessArray<data_type, container_type> test_array_2 = RandomArray(n);
  //  data_type test_array_2 = 0;

  //  // sanity check that all values equal 0
  //  for (std::size_t i = 0; i < n; ++i)
  //  {
  //    ASSERT_TRUE(test_array[i] == 0);
  //  }

  // check that sign(0) = 0
  test_array_2.L2Norm(test_array);
  //  std::cout << test_array_2[0] << std::endl;

  //  data_type test_sum = 0;
  //  for (std::size_t i = 0; i < n; ++i)
  //  {
  //    test_sum += test_array.At(i) * test_array.At(i);
  //  }
  //  test_sum /= 2;
  //  std::cout << test_sum << std::endl;

  // ASSERT_TRUE(test_array_2[i] == 0);
}
