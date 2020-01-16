#pragma once
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

#include "ml/core/subgraph.hpp"
#include "ml/ops/activation.hpp"
#include "ml/ops/weights.hpp"

namespace fetch {

namespace ml {

namespace layers {

template <class T>
class Convolution1D : public SubGraph<T>
{
public:
  using TensorType    = T;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerConvolution1DSaveableParams<TensorType>;

  Convolution1D() = default;

  /**
   * Creates 1D convolution layer with trainable kernel
   * @param output_channels number of output channels
   * @param input_channels number of input channels
   * @param kernel_size size of kernel
   * @param stride_size step size
   * @param activation_type type of activation function applied after convolution
   * @param name name of graph ops
   * @param init_mode mode in which wights(kernel) will be initialised
   * @param seed random seed for weights(kernel) initialisation
   */
  Convolution1D(SizeType const output_channels, SizeType const input_channels,
                SizeType const kernel_size, SizeType const stride_size,
                details::ActivationType const activation_type = details::ActivationType::NOTHING,
                std::string const &           name            = "Conv1D",
                WeightsInit const             init_mode       = WeightsInit::XAVIER_GLOROT,
                SizeType const                seed            = 123456789);

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void SetOpSaveableParams(SPType const &sp);

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_CONVOLUTION_1D;
  }

  static constexpr char const *DESCRIPTOR = "Convolution1DLayer";

private:
  SizeType kernel_size_{};
  SizeType input_channels_{};
  SizeType output_channels_{};
  SizeType stride_size_{};
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
