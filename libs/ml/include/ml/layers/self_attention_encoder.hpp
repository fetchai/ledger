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
class SelfAttentionEncoder : public SubGraph<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using DataType      = typename T::Type;
  using VecTensorType = typename SubGraph<T>::VecTensorType;

  using RegType         = fetch::ml::RegularisationType;
  using WeightsInitType = fetch::ml::ops::WeightsInitialisation;
  using ActivationType  = fetch::ml::details::ActivationType;
  using SPType          = LayerSelfAttentionEncoderSaveableParams<T>;

  SelfAttentionEncoder() = default;

  SelfAttentionEncoder(SizeType n_heads, SizeType model_dim, SizeType ff_dim,
                       DataType       residual_dropout    = fetch::math::Type<DataType>("0.1"),
                       DataType       attention_dropout   = fetch::math::Type<DataType>("0.1"),
                       DataType       feedforward_dropout = fetch::math::Type<DataType>("0.1"),
                       DataType       epsilon         = fetch::math::function_tolerance<DataType>(),
                       ActivationType activation_type = ActivationType::GELU);

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void SetOpSaveableParams(SPType const &sp);

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_SELF_ATTENTION_ENCODER;
  }

  static constexpr char const *DESCRIPTOR = "SelfAttentionEncoder";

  OpType OperationType() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return this->OpCode();
  }
  char const *Descriptor() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return DESCRIPTOR;
  }

private:
  SizeType       n_heads_{};
  SizeType       model_dim_{};
  SizeType       ff_dim_{};
  DataType       residual_dropout_{};
  DataType       attention_dropout_{};
  DataType       feedforward_dropout_{};
  DataType       epsilon_{};
  ActivationType activation_type_;

  std::string positionwise_feedforward(std::string const &name, std::string const &input);

  std::string residual_connection(std::string const &name, std::string const &prev_layer_input,
                                  std::string const &prev_layer_output);
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
