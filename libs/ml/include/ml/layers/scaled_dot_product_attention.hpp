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

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class ScaledDotProductAttention : public SubGraph<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using DataType      = typename T::Type;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerScaledDotProductAttentionSaveableParams<T>;

  ScaledDotProductAttention() = default;
  explicit ScaledDotProductAttention(SizeType dk,
                                     DataType dropout = fetch::math::Type<DataType>("0.1"));

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void SetOpSaveableParams(SPType const &sp);

  inline std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return {inputs.at(2)->shape(0), inputs.front()->shape(1), inputs.front()->shape(2)};
  }

  inline static constexpr OpType OpCode()
  {
    return OpType::LAYER_SCALED_DOT_PRODUCT_ATTENTION;
  }

  static constexpr char const *DESCRIPTOR = "ScaledDotProductAttention";

  OpType OperationType() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return this->OpCode();
  }
  char const *Descriptor() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return DESCRIPTOR;
  }

private:
  SizeType key_dim_{};
  DataType dropout_{};
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
