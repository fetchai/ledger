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

#include "ml/layers/normalisation/layer_norm.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/add.hpp"
#include "ml/ops/multiply.hpp"
#include "ml/ops/weights.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <typename TensorType>
LayerNorm<TensorType>::LayerNorm(std::vector<SizeType> data_shape, SizeType axis, DataType epsilon)
  : data_shape_(std::move(data_shape))
  , axis_(axis)
  , epsilon_(epsilon)
{
  // the data_shape is the shape of the data without including the batch dims
  // make sure we do not do normalization along the batch dims
  ASSERT(axis_ != data_shape_.size());

  // Due to constrain in Add and Multiply layer, we do not support data of more than 3 dims
  ASSERT(data_shape_.size() <= 2);

  std::string name = DESCRIPTOR;

  // instantiate gamma and beta (the multiplicative training component)
  std::string gamma =
      this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Gamma", {});
  std::string beta =
      this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Beta", {});

  // initialization: gamma to all 1, beta to all zero, and with corresponding shape
  std::vector<SizeType> weight_shape(data_shape_.size() + 1, 1);
  weight_shape[axis_] = data_shape_[axis_];
  TensorType gamma_data(weight_shape), beta_data(weight_shape);
  gamma_data.Fill(DataType{1});
  this->SetInput(gamma, gamma_data);
  this->SetInput(beta, beta_data);

  // setup input
  std::string input =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

  // do the normalization
  std::string normalized_output = this->template AddNode<fetch::ml::ops::LayerNorm<TensorType>>(
      name + "_LayerNorm", {input}, axis_, epsilon_);

  // do the rescaling
  std::string scaled_output = this->template AddNode<fetch::ml::ops::Multiply<TensorType>>(
      name + "_Gamma_Multiply", {normalized_output, gamma});

  // do the re-shifting
  std::string shifted_output = this->template AddNode<fetch::ml::ops::Add<TensorType>>(
      name + "_Beta_Addition", {scaled_output, beta});

  this->AddInputNode(input);
  this->SetOutputNode(shifted_output);

  this->Compile();
}

template <typename TensorType>
std::shared_ptr<OpsSaveableParams> LayerNorm<TensorType>::GetOpSaveableParams()
{
  // get all base classes saveable params
  std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

  auto ret = std::make_shared<SPType>();

  // copy subgraph saveable params over
  auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
  auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
  *sg_ptr2     = *sg_ptr1;

  // asign layer specific params
  ret->data_shape = data_shape_;
  ret->axis       = axis_;
  ret->epsilon    = epsilon_;

  return ret;
}

template <typename TensorType>
void LayerNorm<TensorType>::SetOpSaveableParams(SPType const &sp)
{
  // assign layer specific params
  data_shape_ = sp.data_shape;
  axis_       = sp.axis;
  epsilon_    = sp.epsilon;
}

// template class LayerNorm<math::Tensor<int8_t>>;
// template class LayerNorm<math::Tensor<int16_t>>;
template class LayerNorm<math::Tensor<int32_t>>;
template class LayerNorm<math::Tensor<int64_t>>;
// template class LayerNorm<math::Tensor<uint8_t>>;
// template class LayerNorm<math::Tensor<uint16_t>>;
template class LayerNorm<math::Tensor<uint32_t>>;
template class LayerNorm<math::Tensor<uint64_t>>;
template class LayerNorm<math::Tensor<float>>;
template class LayerNorm<math::Tensor<double>>;
template class LayerNorm<math::Tensor<fixed_point::fp32_t>>;
template class LayerNorm<math::Tensor<fixed_point::fp64_t>>;
template class LayerNorm<math::Tensor<fixed_point::fp128_t>>;

}  // namespace layers
}  // namespace ml
}  // namespace fetch
