//#include <cmath>
#include <iomanip>
#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

#include "ml/layers/layer.hpp"

using namespace fetch::ml::layers;

using data_type            = double;
using container_type       = fetch::memory::SharedArray<data_type>;
using vector_register_type = typename container_type::vector_register_type;
#define N 200

//NDArray<data_type, container_type> RandomArray(std::size_t n, std::size_t m)
//{
//  static fetch::random::LinearCongruentialGenerator gen;
//  NDArray<data_type, container_type>                a1(n);
//  for (std::size_t i = 0; i < n; ++i)
//  {
//    a1(i) = data_type(gen.AsDouble());
//  }
//  return a1;
//}
//
//template <typename D>
//using _S = fetch::memory::SharedArray<D>;
//
//template <typename D>
//using _A = NDArray<D, _S<D>>;

TEST(ndarray, layer_assignment)
{


}

TEST(ndarray, faulty_reshape)
{


}