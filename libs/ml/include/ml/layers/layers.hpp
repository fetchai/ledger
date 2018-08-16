#pragma once
#include "ml/layers/base_layer.hpp"
#include "core/random/lcg.hpp"

namespace fetch {
namespace ml {
namespace layers {


/**
 * The base layer class.
 *
 * In general a layer has a size indicating the number of neurons
 *
 *
 */
class Layer : BaseLayer
{
 public:

  /**
   * Constructor that accepts the previous layer as the input to this layer, and the layer size
   * @param input_layer
   */
  Layer(Layer &input_layer, std::size_t layer_size)
  {
    // assign known sizes
    layer_size_ = layer_size;
    input_layer_size_ = input_layer.LayerSize();
    weights_matrix_shape_ = {InputLayerSize(), LayerSize()};

    // instantiate the weights matrix
    weights_matrix_ = NDArray(weights_matrix_shape_);

  }
  /**
   * Constructor that accepts multiple previous layers as the inputs to this layer, and the layer size
   * @param input_layers
   */
  Layer(std::vector<Layer &> input_layers, std::size_t layer_size)
  {
    layer_size_ = layer_size;
    input_layer_size_ = 0;
    for (Layer &cur_layer : input_layers)
    {
      input_layer_size += cur_layer.LayerSize();
    }
    weights_matrix_shape_ = {InputLayerSize(), LayerSize()};
    weights_matrix_ = NDArray(weights_matrix_shape_);

  }


  /**
   * helper function - returns the size of the inputs to this layer
   * @return
   */
  std::size_t InputLayerSize() { return input_layer_size_; }

  /**
 * helper function - returns the shape of the weights matrix
 * @return
 */
  std::size_t WeightsMatrixShape() { return weights_matrix_shape_; }


 private:
  /**
   * initialises the weights matrix with random number from -0.01 to 0.01
   */
  void InitialiseWeightsMatrix()
  {
    // TODO: I image there's a great deal of room for optimisation here

    for (std::size_t i = 0; i < Size(); ++i)
    {


    }

  }

  std::vector<std::size_t> weights_matrix_shape_;
  NDArray weights_matrix_;
}

//  static fetch::random::LinearCongruentialGenerator gen;
//  NDArray<data_type, container_type>                a1(n);
//  for (std::size_t i = 0; i < n; ++i)
//  {
//    a1(i) = data_type(gen.AsDouble());
//  }
//  return a1;










};
};
};