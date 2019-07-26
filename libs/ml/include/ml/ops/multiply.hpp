#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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
class Multiply : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = SaveableParams;

  Multiply() = default;

  explicit Multiply(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Multiply() override = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<SPType>(sp);
  }

  /**
   * elementwise multiplication
   * @param inputs  left & right inputs to multiply
   * @return
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::Multiply((*inputs.at(0)), (*inputs.at(1)), output);
  }

  /**
   * elementwise multiplication gradient is:
   * f'(input0)=input1*error_signal
   * f'(input1)=input0*error_signal
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());
    assert(error_signal.size() == inputs.at(1)->size());

    ArrayType error_signal_1(inputs.at(0)->shape());
    ArrayType error_signal_2(inputs.at(1)->shape());

    fetch::math::Multiply((*inputs.at(1)), error_signal, error_signal_1);
    fetch::math::Multiply((*inputs.at(0)), error_signal, error_signal_2);

    return {error_signal_1, error_signal_2};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr char const *DESCRIPTOR = "Multiply";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
