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

#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Exp : public fetch::ml::ops::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = ExpSaveableParams<T>;

  Exp() = default;

  explicit Exp(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Exp() override = default;

  std::shared_ptr<SaveableParamsInterface> GetOpSaveableParams() override
  {
    auto sp = std::make_shared<SPType>();
    return sp;
  }

  /**
   * elementwise exp
   * @param inputs vector containing one tensor which is the input tensor to Exp
   * @return
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::Exp((*inputs.at(0)), output);
  }

  /**
   * elementwise exp gradient is:
   * f'(input0)= e^x * error_signal
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    ArrayType ret_error_signal(inputs.at(0)->shape());
    fetch::math::Exp((*inputs.at(0)), ret_error_signal);
    fetch::math::Multiply(error_signal, ret_error_signal, ret_error_signal);

    return {ret_error_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr char const *DESCRIPTOR = "Exp";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
