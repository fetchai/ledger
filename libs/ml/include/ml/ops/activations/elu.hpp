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

#include "ml/ops/ops.hpp"

#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Elu : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpEluSaveableParams<TensorType>;
  using MyType        = Elu<TensorType>;

  explicit Elu(DataType a);

  explicit Elu(SPType const &sp);

  ~Elu() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<SizeType> ComputeOutputShape(
      std::vector<math::SizeVector> const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_ELU;
  }
  static constexpr char const *DESCRIPTOR = "Elu";

  std::pair<OperationsCount, math::SizeVector> ChargeForward(
      std::vector<math::SizeVector> const &input_shapes) override;
  std::pair<OperationsCount, math::SizeVector> ChargeBackward(
      std::vector<math::SizeVector> const &input_shapes) override;

private:
  DataType a_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
