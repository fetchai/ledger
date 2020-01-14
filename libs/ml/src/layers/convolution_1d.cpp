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

#include "ml/layers/convolution_1d.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/saveparams/saveable_params.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace layers {

template <typename TensorType>
Convolution1D<TensorType>::Convolution1D(SizeType const output_channels,
                                         SizeType const input_channels, SizeType const kernel_size,
                                         SizeType const                stride_size,
                                         details::ActivationType const activation_type,
                                         std::string const &name, WeightsInit const init_mode,
                                         SizeType const seed)
  : kernel_size_{kernel_size}
  , input_channels_{input_channels}
  , output_channels_{output_channels}
  , stride_size_{stride_size}
{
  std::string input =
      this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

  std::string weights =
      this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights", {});

  TensorType weights_data(
      std::vector<SizeType>{{output_channels_, input_channels_, kernel_size_, 1}});
  fetch::ml::ops::Weights<TensorType>::Initialise(weights_data, 1, 1, init_mode, seed);
  this->SetInput(weights, weights_data);

  std::string output = this->template AddNode<fetch::ml::ops::Convolution1D<TensorType>>(
      name + "_Conv1D", {input, weights}, stride_size_);

  output = fetch::ml::details::AddActivationNode<TensorType>(activation_type, this,
                                                             name + "_Activation", output);

  this->AddInputNode(input);
  this->SetOutputNode(output);

  this->Compile();
}

template <typename TensorType>
void Convolution1D<TensorType>::SetOpSaveableParams(SPType const &sp)
{
  // assign layer specific params
  kernel_size_     = sp.kernel_size;
  input_channels_  = sp.input_channels;
  output_channels_ = sp.output_channels;
  stride_size_     = sp.stride_size;
}

template <class TensorType>
std::shared_ptr<OpsSaveableParams> Convolution1D<TensorType>::GetOpSaveableParams()
{
  // get all base classes saveable params
  std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

  auto ret = std::make_shared<SPType>();

  // copy subgraph saveable params over
  auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
  auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
  *sg_ptr2     = *sg_ptr1;

  // asign layer specific params
  ret->kernel_size     = kernel_size_;
  ret->input_channels  = input_channels_;
  ret->output_channels = output_channels_;
  ret->stride_size     = stride_size_;

  return ret;
}

template <class TensorType>
std::vector<fetch::math::SizeType> Convolution1D<TensorType>::ComputeOutputShape(
    VecTensorType const &inputs) const
{
  TensorType weights_data(
      std::vector<SizeType>{{output_channels_, input_channels_, kernel_size_, 1}});
  return fetch::ml::ops::Convolution1D<TensorType>(stride_size_)
      .ComputeOutputShape({inputs.at(0), std::make_shared<TensorType>(weights_data)});
}

///////////////////////////////
/// EXPLICIT INSTANTIATIONS ///
///////////////////////////////

// template class Convolution1D<math::Tensor<int8_t>>;
// template class Convolution1D<math::Tensor<int16_t>>;
template class Convolution1D<math::Tensor<int32_t>>;
template class Convolution1D<math::Tensor<int64_t>>;
template class Convolution1D<math::Tensor<float>>;
template class Convolution1D<math::Tensor<double>>;
template class Convolution1D<math::Tensor<fixed_point::fp32_t>>;
template class Convolution1D<math::Tensor<fixed_point::fp64_t>>;
template class Convolution1D<math::Tensor<fixed_point::fp128_t>>;

}  // namespace layers
}  // namespace ml
}  // namespace fetch
