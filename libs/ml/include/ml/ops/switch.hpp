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

#include "core/assert.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Switch : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpSwitchSaveableParams<T>;
  using MyType        = Switch<TensorType>;

  Switch() = default;

  explicit Switch(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Switch() override = default;

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

  /**
   * based on boolean condition, switch between second and third array's element.
   * @param inputs - three inputs, first is condition, second is the then array, third is the else
   * array
   * @return
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 3);
    assert(output.shape() == this->ComputeOutputShape(inputs));
    assert(inputs.at(1)->shape() == inputs.at(2)->shape());

    fetch::math::Switch(*(inputs.at(0)), *(inputs.at(1)), *(inputs.at(2)), output);
  }

  /**
   * elementwise gradient for second input (the then input) is:
   * error' = mask * error_signal
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 3);
    assert(inputs.at(1)->shape() == inputs.at(2)->shape());
    assert(error_signal.size() == inputs.at(1)->size());

    TensorType then_return_signal(inputs.at(1)->shape());
    TensorType else_return_signal(inputs.at(2)->shape());
    TensorType mask_return_signal(inputs.at(0)->shape());

    fetch::math::Multiply(*(inputs.front()), error_signal, then_return_signal);
    fetch::math::Subtract(error_signal, then_return_signal, else_return_signal);

    // be adivsed, it is not reasonable to return gradient for mask, so the mask gradient is set to
    // zero here
    return {mask_return_signal, then_return_signal, else_return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.at(1)->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_SWITCH;
  }

  static constexpr char const *DESCRIPTOR = "Switch";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
