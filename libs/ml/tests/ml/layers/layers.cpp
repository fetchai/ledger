//#include <cmath>
//#include <iomanip>
//#include <iostream>

//#include "core/random/lcg.hpp"
#include <gtest/gtest.h>

#include "ml/layers/layers.hpp"

using namespace fetch::ml::layers;

using data_type      = double;
using container_type = fetch::memory::SharedArray<data_type>;
// using vector_register_type = typename container_type::vector_register_type;
//#define N 200
//
// NDArray<data_type, container_type> RandomArray(std::size_t n, std::size_t m)
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
// template <typename D>
// using _S = fetch::memory::SharedArray<D>;
//
// template <typename D>
// using _A = NDArray<D, _S<D>>;

TEST(ndarray, layer_shapes_test)
{
  // set up a randomly initialised feed forward neural net
  InputLayer<data_type> layer1(5);
  ASSERT_TRUE(layer1.LayerSize() == 5);

  Layer<data_type>      layer2(layer1, 100);
  ASSERT_TRUE(layer2.InputLayerSize() == 5);
  ASSERT_TRUE(layer2.LayerSize() == 100);
  std::vector<std::size_t> test_shape{5, 100};
  ASSERT_TRUE(layer2.WeightsMatrixShape() == test_shape);

  Layer<data_type>      layer3(layer2, 10);
  ASSERT_TRUE(layer3.InputLayerSize() == 100);
  ASSERT_TRUE(layer3.LayerSize() == 10);
  test_shape = {100, 10};
  ASSERT_TRUE(layer3.WeightsMatrixShape() == test_shape);

  Layer<data_type>      layer4(layer3, 1);
  ASSERT_TRUE(layer4.InputLayerSize() == 10);
  ASSERT_TRUE(layer4.LayerSize() == 1);
  test_shape = {10, 1};
  ASSERT_TRUE(layer4.WeightsMatrixShape() == test_shape);

}

// TEST(ndarray, faulty_reshape)
//{
//
//
//}