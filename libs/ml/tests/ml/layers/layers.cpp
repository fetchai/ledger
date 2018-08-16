//#include <cmath>
//#include <iomanip>
//#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

#include "ml/layers/layers.hpp"

using namespace fetch::ml::layers;

using data_type            = double;
using container_type       = fetch::memory::SharedArray<data_type>;
//using vector_register_type = typename container_type::vector_register_type;
//#define N 200
//
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

TEST(ndarray, layer_constructor_tests)
{
  // set up a randomly initialised feed forward neural net
  InputLayer<data_type> layer1(5);
  Layer<data_type> layer2(layer1, 100);
  Layer<data_type> layer3(layer2, 10);
  Layer<data_type> layer4(layer3, 1);

}

//TEST(ndarray, faulty_reshape)
//{
//
//
//}