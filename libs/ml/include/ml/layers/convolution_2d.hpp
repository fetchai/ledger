#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/weights.hpp"
#include "ml/subgraph.hpp"

#include <cmath>
#include <random>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class Convolution2D : public SubGraph<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;
  using WeightsInit  = fetch::ml::ops::WeightsInitialisation;

  Convolution2D(SizeType const output_channels, SizeType const input_channels,
                SizeType const kernel_size, SizeType const stride_size,
                details::ActivationType const activation_type = details::ActivationType::NOTHING,
                std::string const &           name            = "Conv2D",
                WeightsInit const             init_mode       = WeightsInit::XAVIER_GLOROT,
                SizeType const                seed            = 123456789)
    : kernel_size_{kernel_size}
    , input_channels_{input_channels}
    , output_channels_{output_channels}
    , stride_size_{stride_size}
  {
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});

    std::string weights =
        this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Weights", {});

    ArrayType weights_data(
        std::vector<SizeType>{{output_channels_, input_channels_, kernel_size_, kernel_size_}});
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights_data, 1, 1, init_mode, seed);
    this->SetInput(weights, weights_data);

    std::string output = this->template AddNode<fetch::ml::ops::Convolution2D<ArrayType>>(
        name + "_Conv2D", {input, weights}, stride_size_);

    output = fetch::ml::details::AddActivationNode<T>(activation_type, this, name + "_Activation",
                                                      output);

    this->AddInputNode(input);
    this->SetOutputNode(output);
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    // Return pre-computed value if exist
    if (output_shape_.size() != 0)
      return output_shape_;
    // output_shape_[0]=number of output channels
    output_shape_.emplace_back(inputs.at(1).get().shape()[0]);
    // output_shape_[1]=number of stride_size steps on input height
    output_shape_.emplace_back(
        (inputs.at(0).get().shape()[1] - inputs.at(1).get().shape()[2] + stride_size_) /
        stride_size_);
    // output_shape_[2]=number of stride_size steps on input width
    output_shape_.emplace_back(
        (inputs.at(0).get().shape()[2] - inputs.at(1).get().shape()[3] + stride_size_) /
        stride_size_);
    return output_shape_;
  }

  static constexpr char const *DESCRIPTOR = "Convolution2D";

private:
  void Initialise(ArrayType &weights, WeightsInit init_mode)
  {
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, input_channels_, output_channels_,
                                                   init_mode);
  }

  SizeType              kernel_size_;
  SizeType              input_channels_;
  SizeType              output_channels_;
  SizeType              stride_size_;
  std::vector<SizeType> output_shape_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
