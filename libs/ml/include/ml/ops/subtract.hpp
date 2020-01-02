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

#include "core/macros.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Subtract : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpSubtractSaveableParams<T>;
  using MyType        = Subtract<TensorType>;

  Subtract() = default;

  explicit Subtract(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Subtract() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    return std::make_shared<SPType>(sp);
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

    return copyshare;
  }
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::Subtract((*inputs.at(0)), (*inputs.at(1)), output);
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());
    assert(error_signal.size() == inputs.at(1)->size());

    return {error_signal, fetch::math::Multiply(error_signal, DataType{-1})};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_SUBTRACT;
  }
  static constexpr char const *DESCRIPTOR = "Subtract";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
