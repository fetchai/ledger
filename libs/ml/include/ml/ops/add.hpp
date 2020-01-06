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

#include "ml/meta/ml_type_traits.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {

struct OpsSaveableParams;

template <typename TensorType>
struct OpAddSaveableParams;

namespace ops {

template <class T>
class Add : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpAddSaveableParams<T>;
  using MyType        = Add<TensorType>;

  Add() = default;

  explicit Add(SPType const &sp);

  ~Add() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override;

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override;

  // for inputs to the add layer, if broadcasting is required, make sure the first input is the one
  // with the complete shape
  void Forward(VecTensorType const &inputs, TensorType &output) override;

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override;

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override;

  static constexpr OpType OpCode()
  {
    return OpType::OP_ADD;
  }

  static constexpr char const *DESCRIPTOR = "Add";

private:
  std::vector<SizeType> axes_;

  void UpdateAxes(VecTensorType const &inputs);
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
