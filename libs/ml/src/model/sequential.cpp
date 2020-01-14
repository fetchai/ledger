//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "ml/model/sequential.hpp"

namespace fetch {
namespace ml {
namespace model {

/**
 * constructor sets up the neural net architecture and optimiser with default config
 * @tparam TensorType
 * @param data_loader_ptr  pointer to the dataloader for running the optimiser
 * @param hidden_layers vector of hidden layers for defining the architecture
 * @param optimiser_type type of optimiser to run
 */
template <typename TensorType>
Sequential<TensorType>::Sequential()
  : Model<TensorType>(ModelConfig<DataType>())
{
  this->input_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Input", {});
  this->label_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Label", {});
}

/**
 * constructor sets up the neural net architecture and optimiser
 * @tparam TensorType
 * @param data_loader_ptr  pointer to the dataloader for running the optimiser
 * @param model_config config parameters for setting up the network
 * @param hidden_layers vector of hidden layers for defining the architecture
 * @param optimiser_type type of optimiser to run
 */
template <typename TensorType>
Sequential<TensorType>::Sequential(ModelConfig<DataType> model_config)
  : Model<TensorType>(model_config)
{
  this->input_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Input", {});
  this->label_ = this->graph_ptr_->template AddNode<ops::PlaceHolder<TensorType>>("Label", {});
}

template <typename TensorType>
fetch::math::SizeType Sequential<TensorType>::LayerCount() const
{
  return layer_count_;
}
///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////
// ML-464
// template class Sequential<math::Tensor<int8_t>>;
template class Sequential<math::Tensor<int16_t>>;
template class Sequential<math::Tensor<int32_t>>;
template class Sequential<math::Tensor<int64_t>>;
template class Sequential<math::Tensor<float>>;
template class Sequential<math::Tensor<double>>;
template class Sequential<math::Tensor<fixed_point::fp32_t>>;
template class Sequential<math::Tensor<fixed_point::fp64_t>>;
template class Sequential<math::Tensor<fixed_point::fp128_t>>;
}  // namespace model
}  // namespace ml
}  // namespace fetch
