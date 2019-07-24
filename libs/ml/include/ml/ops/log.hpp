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

#include "math/standard_functions/log.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Log : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  Log()           = default;
  ~Log() override = default;

  /**
   * elementwise Log
   * @param inputs vector containing one tensor which is the input tensor to Log
   * @return
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::Log(inputs.at(0).get(), output);
  }

  /**
   * elementwise log gradient is 1/x * error:
   * f'(input0)= error_signal/input0
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    ArrayType ret_error_signal(inputs.at(0).get().shape());
    fetch::math::Divide(error_signal, inputs.at(0).get(), ret_error_signal);

    return {ret_error_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Log";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
