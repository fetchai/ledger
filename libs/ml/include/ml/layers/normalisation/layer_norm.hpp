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
class LayerNorm : public SubGraph<T>
{
public:
  using TensorType    = T;
  using TensorPtrType = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using DataType      = typename TensorType::Type;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerLayerNormSaveableParams<T>;

  LayerNorm() = default;

  explicit LayerNorm(std::vector<SizeType> data_shape, SizeType axis = static_cast<SizeType>(0),
                     DataType epsilon = fetch::math::function_tolerance<DataType>());

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void SetOpSaveableParams(SPType const &sp);

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return {inputs.at(0)->shape()};
  }

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_LAYER_NORM;
  }

  static constexpr char const *DESCRIPTOR = "LayerNorm";

  OpType OperationType() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return this->OpCode();
  }
  char const *Descriptor() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return DESCRIPTOR;
  }

private:
  std::vector<SizeType> data_shape_;
  SizeType              axis_{};
  DataType              epsilon_{};
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
