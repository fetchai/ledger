#pragma once

#include "math/ndarray.hpp"

namespace fetch {
namespace ml {
namespace layers {

class Layer : public NDArray<T, C>
{
 public:

  /**
   *
   * @param input_layer
   */
  Layer(Layer &input_layer, std::size_t layer_size)
  {
    layer_size_ = layer_size;

    weights_matrix_shape_ = {input_layer.LayerSize(), layer_size()};
    weights_matrix_ = NDArray(weights_matrix_shape_);

  }
  Layer(std::vector<Layer &> input_layers){}

  std::size_t LayerSize() { return layer_size_; }

 private:
  std::size_t layer_size_;

  std::vector<std::size_t> weights_matrix_shape_;
  NDArray weights_matrix_;
}

};
};
};