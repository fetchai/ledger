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
#include "ml/ops/weights.hpp"

namespace fetch {
namespace ml {
namespace layers {

template <class T>
class PRelu : public SubGraph<T>
{
public:
  using TensorType    = T;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using SizeType      = fetch::math::SizeType;
  using WeightsInit   = fetch::ml::ops::WeightsInitialisation;
  using VecTensorType = typename SubGraph<T>::VecTensorType;
  using SPType        = LayerPReluSaveableParams<TensorType>;

  PRelu() = default;

  explicit PRelu(uint64_t in, std::string const &name = "PRelu",
                 WeightsInit init_mode = WeightsInit::XAVIER_GLOROT);

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  void SetOpSaveableParams(SPType const &sp)
  {
    FETCH_UNUSED(sp);
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return {inputs.at(0)->shape()};
  }

  static constexpr OpType OpCode()
  {
    return OpType::LAYER_PRELU;
  }

  static constexpr char const *DESCRIPTOR = "ParametricRelu";

  OpType OperationType() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return this->OpCode();
  }
  char const *Descriptor() const override  // TODO(ML-466) : move implementation to .cpp
  {
    return DESCRIPTOR;
  }
};

}  // namespace layers
}  // namespace ml
}  // namespace fetch
