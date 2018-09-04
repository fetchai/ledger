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

#include "math/ndarray.hpp"
#include "vectorise/memory/array.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <typename DATA_TYPE>
class BaseLayer
{

  using container_type = memory::SharedArray<DATA_TYPE>;
  using array_type     = math::NDArray<DATA_TYPE, container_type>;

public:
  BaseLayer()
  {}
  BaseLayer(std::size_t layer_size)
  {
    layer_size_ = layer_size;
  }

  BaseLayer &operator=(BaseLayer const &other) = default;

  void SetLayerSize(std::size_t layer_size)
  {
    layer_size_ = layer_size;
  }
  std::size_t LayerSize()
  {
    return layer_size_;
  }
  void SetWeightsMatrix(array_type new_weights_matrix)
  {
    weights_matrix_ = new_weights_matrix;
  }
  array_type WeightsMatrix()
  {
    return weights_matrix_;
  }

protected:
  std::size_t layer_size_;
  array_type  weights_matrix_;

private:
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch