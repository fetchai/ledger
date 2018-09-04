#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/random/lcg.hpp"
#include "ml/layers/base_layer.hpp"

namespace fetch {
namespace ml {
namespace layers {

/**
 * Special layer type that feeds data into the network; it has no weights matrix
 */
template <typename DATA_TYPE>
class InputLayer : public BaseLayer<DATA_TYPE>
{
  using container_type = memory::SharedArray<DATA_TYPE>;
  using base_type      = BaseLayer<DATA_TYPE>;

public:
  InputLayer(std::size_t layer_size) : base_type::BaseLayer(layer_size)
  {}

  void AssignData(std::vector<DATA_TYPE> input_data)
  {
    input_data_ = input_data;
  }

private:
  std::vector<DATA_TYPE> input_data_;
};

/**
 * The base layer class.
 *
 * In general a layer has a size indicating the number of neurons
 *
 *
 */
// template<typename DATA_TYPE, typename CONTAINER_TYPE = memory::SharedArray<DATA_TYPE>>
template <typename DATA_TYPE>
class Layer : public BaseLayer<DATA_TYPE>
{
  using container_type = memory::SharedArray<DATA_TYPE>;
  using base_type      = BaseLayer<DATA_TYPE>;
  using array_type     = math::NDArray<DATA_TYPE, container_type>;

public:
  /**
   * Constructor that accepts the previous layer as the input to this layer, and the layer size
   * @param input_layer
   */
  Layer &operator=(Layer const &other) = default;
  Layer(InputLayer<DATA_TYPE> &input_layer, std::size_t layer_size) : base_type(layer_size)
  {
    AssignLayerConnection(input_layer, layer_size);
  }
  Layer(Layer &input_layer, std::size_t layer_size)
  {
    AssignLayerConnection(input_layer, layer_size);
  }

  /**
   * helper function - returns the size of the inputs to this layer
   * @return
   */
  std::size_t InputLayerSize()
  {
    return input_layer_size_;
  }

  /**
   * helper function - returns the shape of the weights matrix
   * @return
   */
  std::vector<std::size_t> WeightsMatrixShape()
  {
    return weights_matrix_shape_;
  }

private:
  /**
   * initialises the weights matrix with random number from -0.01 to 0.01
   */
  std::size_t              input_layer_size_;
  std::vector<std::size_t> weights_matrix_shape_;

  template <typename LAYER_TYPE>
  void AssignLayerConnection(LAYER_TYPE input_layer, std::size_t new_layer_size)
  {
    // assign known sizes
    this->SetLayerSize(new_layer_size);
    input_layer_size_     = input_layer.LayerSize();
    weights_matrix_shape_ = {InputLayerSize(), this->LayerSize()};

    // instantiate the weights matrix
    this->weights_matrix_ = ndarray_type(WeightsMatrixShape());
    this->SetWeightsMatrix(this->weights_matrix_);
  }

  void InitialiseWeightsMatrix()
  {
    array_type arr{this->Size()};

    for (std::size_t i = 0; i < this->Size(); ++i)
    {
      arr[i] = 0;
    }
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch