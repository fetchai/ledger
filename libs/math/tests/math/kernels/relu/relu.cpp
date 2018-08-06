#include <iomanip>
#include <iostream>

#include <gtest/gtest.h>
#include "math/kernels/relu.hpp"
#include "math/shape_less_array.hpp"


using namespace fetch::math;
using data_type = double;
using container_type = fetch::memory::SharedArray<data_type>;

ShapeLessArray<data_type, container_type> RandomArray(std::size_t n, std::size_t m)
{
  static fetch::random::LinearCongruentialGenerator gen;
  ShapeLessArray<data_type, container_type> a1(n);
  for (std::size_t i = 0; i < n; ++i)
  {
    a1.At(i) = data_type(gen.AsDouble()) - 1.0;
  }
  return a1;
}


TEST(ndarray, zeros_out) {

  std::size_t n = 1000;
  ShapeLessArray<data_type, container_type> test_array = RandomArray(n, n);
  ShapeLessArray<data_type, container_type> test_array_2 = RandomArray(n, n);

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i) {
    ASSERT_TRUE(test_array[i] < 0);
  }

  //
  test_array_2.Relu(test_array);

  // sanity check that all values less than 0
  for (std::size_t i = 0; i < n; ++i) {
    ASSERT_TRUE(test_array_2[i] == 0);
  }

}