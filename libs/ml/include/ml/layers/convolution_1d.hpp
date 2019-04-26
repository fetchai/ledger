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
#include "ml/ops/add.hpp"
#include "ml/ops/convolution_1d.hpp"
#include "ml/ops/flatten.hpp"
#include "ml/ops/leaky_relu_op.hpp"
#include "ml/ops/matrix_multiply.hpp"
#include "ml/ops/weights.hpp"
#include "ml/subgraph.hpp"

#include <cmath>
#include <random>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class Convolution1D : public SubGraph<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;
  using WeightsInit  = fetch::ml::ops::WeightsInitialisation;

  Convolution1D(SizeType output_channels, SizeType input_channels, SizeType kernel_size,
                SizeType                stride_size,
                details::ActivationType activation_type = details::ActivationType::NOTHING,
                std::string const &     name            = "Conv1D",
                WeightsInit             init_mode       = WeightsInit::XAVIER_GLOROT)

    : kernel_size_(kernel_size)
    , input_channels_(input_channels)
    , output_channels_(output_channels)
    , stride_size_(stride_size)
  {
    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<ArrayType>>(name + "_Input", {});

    std::string weights =
        this->template AddNode<fetch::ml::ops::Weights<ArrayType>>(name + "_Weights", {});

    std::string output = this->template AddNode<fetch::ml::ops::Convolution1D<ArrayType>>(
        name + "_Conv1D", {input, weights}, stride_size_);

    output = fetch::ml::details::AddActivationNode<T>(activation_type, this, name + "_Activation",
                                                      output);

    this->AddInputNode(input);
    this->SetOutputNode(output);

    ArrayType weights_data(
        std::vector<SizeType>({output_channels_, input_channels_, kernel_size_}));
    this->Initialise(weights_data, init_mode);
    this->SetInput(weights, weights_data, false, false);
  }

  virtual std::vector<SizeType> ComputeOutputShape(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs) const
  {
    std::vector<typename ArrayType::SizeType> output_shape;
    output_shape.push_back(output_channels_);
    output_shape.push_back((inputs.at(0).get().shape()[1] - kernel_size_ + stride_size_) /
                           stride_size_);
    return output_shape;
  }

  static constexpr char const *DESCRIPTOR = "Convolution1D";

private:
  void Initialise(ArrayType &weights, WeightsInit init_mode)
  {
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights, input_channels_, output_channels_,
                                                   init_mode);
  }

  SizeType kernel_size_;
  SizeType input_channels_;
  SizeType output_channels_;
  SizeType stride_size_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
