//#include <cmath>
#include <iomanip>
#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

#include "math/exp.hpp"
#include "math/kernels/concurrent_vm.hpp"
#include "math/linalg/matrix.hpp"
#include "math/ndarray.hpp"
#include "math/rectangular_array.hpp"
#include "vectorise/threading/pool.hpp"

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