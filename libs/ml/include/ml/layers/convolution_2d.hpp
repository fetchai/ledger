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
class Convolution2D : public SubGraph<T>
{
public:
  using TensorType    = T;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerConvolution2DSaveableParams<TensorType>;

  Convolution2D() = default;

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
  Convolution2D(SizeType output_channels, SizeType input_channels, SizeType kernel_size,
                SizeType                stride_size,
                details::ActivationType activation_type = details::ActivationType::NOTHING,
                std::string const &     name            = "Conv2D",
                WeightsInit init_mode = WeightsInit::XAVIER_GLOROT, SizeType seed = 123456789);

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void SetOpSaveableParams(SPType const &sp);

  void CompleteShapeDeduction() override;

  std::vector<SizeType> ComputeOutputShape(
      std::vector<math::SizeVector> const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_CONVOLUTION_2D;
  }

  static constexpr char const *DESCRIPTOR = "Convolution2DLayer";

  OpType OperationType() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return this->OpCode();
  }
  char const *Descriptor() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return DESCRIPTOR;
  }

  void Compile() override;

  OperationsCount ChargeBackward() const override;

  OperationsCount ChargeCompile() override;

private:
  void Initialise(TensorType &weights, WeightsInit init_mode)
  {
    fetch::ml::ops::Weights<TensorType>::Initialise(weights, input_channels_, output_channels_,
                                                    init_mode);
  }

  SizeType kernel_size_{};
  SizeType input_channels_{};
  SizeType output_channels_{};
  SizeType stride_size_{};
  bool     is_initialised_ = false;

  WeightsInit init_mode_;
  SizeType    seed_;
  std::string weights_;
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
