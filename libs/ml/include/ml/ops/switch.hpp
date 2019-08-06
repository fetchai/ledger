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
class Switch : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  Switch()           = default;
  ~Switch() override = default;

  /**
   * based on boolean condition, switch between second and third array's element.
   * N.B. the backprop is only done on the second array, the third array is only used to spcify the
   * masked value
   * @param inputs - three inputs, first is condition, second is the then array, third is the else
   * array
   * @return
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 3);
    assert(output.shape() == this->ComputeOutputShape(inputs));
    assert(inputs.at(0)->shape() == inputs.at(1)->shape());
    assert(inputs.at(1)->shape() == inputs.at(2)->shape());

    output = inputs.at(2)->Copy();
    fetch::math::Switch(*(inputs.at(0)), *(inputs.at(1)), *(inputs.at(2)), output);
  }

  /**
   * elementwise gradient for second input (the then input) is:
   * error' = mask * error_signal
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 3);
    assert(inputs.at(0)->shape() == inputs.at(1)->shape());
    assert(inputs.at(1)->shape() == inputs.at(2)->shape());
    assert(error_signal.size() == inputs.at(0)->size());

    ArrayType then_return_signal(inputs.at(0)->shape());
    ArrayType else_return_signal(inputs.at(0)->shape());
    ArrayType mask_return_signal(inputs.at(0)->shape());

    fetch::math::Multiply(*(inputs.front()), error_signal, then_return_signal);
    fetch::math::Subtract(error_signal, then_return_signal, else_return_signal);

    // be adivsed, it is not reasonable to return gradient for mask, so the mask gradient is set to
    // zero here
    return {mask_return_signal, then_return_signal, else_return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr char const *DESCRIPTOR = "Switch";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
