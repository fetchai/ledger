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

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class Convolution2D : public SubGraph<T>
{
public:
  using ArrayType     = T;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using SizeType      = typename ArrayType::SizeType;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = ConvolutionLayerSaveableParams<ArrayType>;

  /**
   * Creates 2D convolution layer with trainable kernel
   * @param output_channels number of output channels
   * @param input_channels number of input channels
   * @param kernel_size size of kernel
   * @param stride_size step size
   * @param activation_type type of activation function applied after convolution
   * @param name name of graph ops
   * @param init_mode mode in which wights(kernel) will be initialised
   * @param seed random seed for weights(kernel) initialisation
   */
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
        std::vector<SizeType>{{output_channels_, input_channels_, kernel_size_, kernel_size_, 1}});
    fetch::ml::ops::Weights<ArrayType>::Initialise(weights_data, 1, 1, init_mode, seed);
    this->SetInput(weights, weights_data);

    std::string output = this->template AddNode<fetch::ml::ops::Convolution2D<ArrayType>>(
        name + "_Conv2D", {input, weights}, stride_size_);

    output = fetch::ml::details::AddActivationNode<T>(activation_type, this, name + "_Activation",
                                                      output);

    this->AddInputNode(input);
    this->SetOutputNode(output);
  }

  explicit Convolution2D(SPType const &gs)
    : SubGraph<ArrayType>(gs)
  {
    kernel_size_     = gs.kernel_size;
    input_channels_  = gs.input_channels;
    output_channels_ = gs.output_channels;
    stride_size_     = gs.stride_size;
  }

  std::shared_ptr<SaveableParamsInterface> GetOpSaveableParams() override
  {
    std::shared_ptr<SaveableParamsInterface> sgsp = SubGraph<ArrayType>::GetOpSaveableParams();
    auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<ArrayType>::SPType>(sgsp);

    auto sp      = std::make_shared<SPType>();
    auto sg_ptr2 = std::static_pointer_cast<typename SubGraph<ArrayType>::SPType>(sp);
    *sg_ptr2     = *sg_ptr1;

    sp->kernel_size     = kernel_size_;
    sp->input_channels  = input_channels_;
    sp->output_channels = output_channels_;
    sp->stride_size     = stride_size_;
    return sp;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    ArrayType weights_data(
        std::vector<SizeType>{{output_channels_, input_channels_, kernel_size_, kernel_size_, 1}});
    return fetch::ml::ops::Convolution2D<ArrayType>(stride_size_)
        .ComputeOutputShape({inputs.at(0), std::make_shared<ArrayType>(weights_data)});
  }

  static constexpr char const *DESCRIPTOR = "Convolution2DLayer";

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
