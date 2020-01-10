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

namespace fetch {
namespace ml {
namespace layers {

template <class T>
Convolution1D<T>::Convolution1D(SizeType const output_channels, SizeType const input_channels,
                SizeType const kernel_size, SizeType const stride_size,
                details::ActivationType const activation_type = details::ActivationType::NOTHING,
                std::string const &           name            = "Conv1D",
                WeightsInit const             init_mode       = WeightsInit::XAVIER_GLOROT,
                SizeType const                seed            = 123456789)
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

    output = fetch::ml::details::AddActivationNode<T>(activation_type, this, name + "_Activation",
                                                      output);

    this->AddInputNode(input);
    this->SetOutputNode(output);

    this->Compile();
  }

  ///////////////////////////////
  /// EXPLICIT INSTANTIATIONS ///
  ///////////////////////////////

  template class Convolution1D<math::Tensor<int8_t>>;
  template class Convolution1D<math::Tensor<int16_t>>;
  template class Convolution1D<math::Tensor<int32_t>>;
  template class Convolution1D<math::Tensor<int64_t>>;
  template class Convolution1D<math::Tensor<uint8_t>>;
  template class Convolution1D<math::Tensor<uint16_t>>;
  template class Convolution1D<math::Tensor<uint32_t>>;
  template class Convolution1D<math::Tensor<uint64_t>>;
  template class Convolution1D<math::Tensor<float>>;
  template class Convolution1D<math::Tensor<double>>;
  template class Convolution1D<math::Tensor<fixed_point::fp32_t>>;
  template class Convolution1D<math::Tensor<fixed_point::fp64_t>>;
  template class Convolution1D<math::Tensor<fixed_point::fp128_t>>;

}
}
}



