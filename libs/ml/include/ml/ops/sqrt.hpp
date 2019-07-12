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
class Sqrt : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  Sqrt()           = default;
  ~Sqrt() override = default;

  /**
   * elementwise square root
   * @param inputs vector containing one tensor which is the input tensor to Sqrt
   * @return
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::Sqrt(inputs.at(0).get(), output);
  }

  /**
   * elementwise square root gradient is:
   * f'(input0)= 0.5 * (input0 ^ -0.5) * error_signal
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    ArrayType ret_error_signal(inputs.at(0).get().shape());

    fetch::math::Sqrt(inputs.at(0).get(), ret_error_signal);
    fetch::math::Divide(static_cast<DataType>(0.5), ret_error_signal, ret_error_signal);
    fetch::math::Multiply(error_signal, ret_error_signal, ret_error_signal);

    return {ret_error_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Sqrt";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
