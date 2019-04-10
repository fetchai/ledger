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

#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/ml/activation_functions/sigmoid.hpp"
#include "math/standard_functions/exp.hpp"
#include "ml/ops/ops.hpp"
#include <math/standard_functions/log.hpp>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LogSigmoid : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  LogSigmoid()          = default;
  virtual ~LogSigmoid() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    assert(inputs.size() == 1);
    ASSERT(output.shape() == this->ComputeOutputSize(inputs));

    fetch::math::Sigmoid(inputs.front().get(), output);
    fetch::math::Log(output, output);

    // ensures numerical stability
    for (auto &val : output)
    {
      fetch::math::Min(val, epsilon_, val);
    }

    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == errorSignal.shape());
    ArrayType returnSignal{errorSignal.shape()};

    // gradient of log-sigmoid function is 1/(e^x + 1))
    fetch::math::Add(fetch::math::Exp(inputs.front().get()), DataType(1), returnSignal);
    fetch::math::Divide(DataType(1), returnSignal, returnSignal);

    // multiply by errorSignal (chain rule)
    fetch::math::Multiply(errorSignal, returnSignal, returnSignal);

    return {returnSignal};
  }

  static constexpr char const *DESCRIPTOR = "LogSigmoid";

private:
  // maximum possible output value of the log-sigmoid should not be zero, but actually epsilon
  DataType epsilon_ = DataType(1e-12);
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
