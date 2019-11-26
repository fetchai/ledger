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

#include "ml/core/subgraph.hpp"
#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/concatenate.hpp"
#include "ml/ops/convolution_2d.hpp"
#include "ml/ops/placeholder.hpp"
#include "ml/ops/slice.hpp"
#include "ml/ops/weights.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class DepthwiseConv2D : public SubGraph<T>
{
public:
  using TensorType    = T;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerDepthwiseConv2DSaveableParams<TensorType>;

  DepthwiseConv2D() = default;

  /**
   * Creates depthwise 2D convolution layer
   * @param output_channels number of output channels
   * @param input_channels number of input channels
   * @param kernel_size size of kernel
   * @param stride_size step size
   * @param activation_type type of activation function applied after convolution
   * @param name name of graph ops
   * @param init_mode mode in which wights(kernel) will be initialised
   * @param seed random seed for weights(kernel) initialisation
   */
  DepthwiseConv2D(SizeType const input_channels, SizeType const kernel_size,
                  SizeType const stride_size, SizeType depth_multiplier,
                  details::ActivationType const activation_type = details::ActivationType::NOTHING,
                  std::string const &           name            = "DepthwiseConv2D",
                  WeightsInit const             init_mode       = WeightsInit::XAVIER_GLOROT,
                  SizeType const                seed            = 123456789)
    : kernel_size_{kernel_size}
    , input_channels_{input_channels}
    , depth_multiplier_{depth_multiplier}
    , stride_size_{stride_size}
  {
    SizeType channel_dim = 0;

    std::string input =
        this->template AddNode<fetch::ml::ops::PlaceHolder<TensorType>>(name + "_Input", {});

    std::string aggregated_activation;
    for (std::size_t i = 0; i < input_channels_; ++i)
    {
      // slice out the input data by channel
      std::string slice_by_channel = this->template AddNode<fetch::ml::ops::Slice<TensorType>>(
          name + "_Slice" + std::to_string(i), {input}, i, channel_dim);

      for (std::size_t j = 0; j < depth_multiplier_; ++j)
      {
        // set up a kernel (height * weight * 1)
        std::string weights =
            this->template AddNode<fetch::ml::ops::Weights<TensorType>>(name + "_Weights" + std::to_string(i) + "_" + std::to_string(j), {});

        TensorType weights_data(std::vector<SizeType>{{1, 1, kernel_size_, kernel_size_, 1}});
        fetch::ml::ops::Weights<TensorType>::Initialise(weights_data, 1, 1, init_mode, seed);
        this->SetInput(weights, weights_data);

        std::string activation = this->template AddNode<fetch::ml::ops::Convolution2D<TensorType>>(
            name + "_Conv2D_" + std::to_string(i) + "_" + std::to_string(j), {slice_by_channel, weights}, stride_size_);

        aggregated_activation = this->template AddNode<fetch::ml::ops::Concatenate<TensorType>>(
            name + "_Concat_" + std::to_string(i) + "_" + std::to_string(j), {activation, weights}, static_cast<SizeType>(0));
      }
    }

    std::string output = fetch::ml::details::AddActivationNode<T>(
        activation_type, this, name + "_Activation", aggregated_activation);

    this->AddInputNode(input);
    this->SetOutputNode(output);

    this->Compile();
  }

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    // get all base classes saveable params
    std::shared_ptr<OpsSaveableParams> sgsp = SubGraph<TensorType>::GetOpSaveableParams();

    auto ret = std::make_shared<SPType>();

    // copy subgraph saveable params over
    auto sg_ptr1 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(sgsp);
    auto sg_ptr2 = std::dynamic_pointer_cast<typename SubGraph<TensorType>::SPType>(ret);
    *sg_ptr2     = *sg_ptr1;

    // asign layer specific params
    ret->kernel_size      = kernel_size_;
    ret->input_channels   = input_channels_;
    ret->depth_multiplier = depth_multiplier_;
    ret->stride_size      = stride_size_;

    return ret;
  }

  void SetOpSaveableParams(SPType const &sp)
  {
    // assign layer specific params
    kernel_size_      = sp.kernel_size;
    input_channels_   = sp.input_channels;
    depth_multiplier_ = sp.depth_multiplier;
    stride_size_      = sp.stride_size;
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    TensorType weights_data(
        std::vector<SizeType>{{depth_multiplier_, input_channels_, kernel_size_, kernel_size_, 1}});
    return fetch::ml::ops::Convolution2D<TensorType>(stride_size_)
        .ComputeOutputShape({inputs.at(0), std::make_shared<TensorType>(weights_data)});
  }

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_DEPTHWISE_CONV_2D;
  }

  static constexpr char const *DESCRIPTOR = "DepthwiseConvolution2DLayer";

private:
  SizeType kernel_size_{};
  SizeType input_channels_{};
  SizeType depth_multiplier_{};
  SizeType stride_size_{};
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
