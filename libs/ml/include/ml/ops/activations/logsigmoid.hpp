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
#include "math/standard_functions/log.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LogSigmoid : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  LogSigmoid()          = default;
  virtual ~LogSigmoid() = default;

  std::shared_ptr<SaveableParams<ArrayType>> GetOpSaveableParams ()
  {
    SaveableParams<ArrayType> sp{};
    sp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<SaveableParams<ArrayType>>(sp);
  }

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::Sigmoid(inputs.front().get(), output);
    fetch::math::Log(output, output);

    // ensures numerical stability
    for (auto &val : output)
    {
      fetch::math::Min(val, epsilon_, val);
    }
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == error_signal.shape());
    ArrayType return_signal{error_signal.shape()};

    // gradient of log-sigmoid function is 1/(e^x + 1))
    fetch::math::Add(fetch::math::Exp(inputs.front().get()), static_cast<DataType>(1),
                     return_signal);
    fetch::math::Divide(static_cast<DataType>(1), return_signal, return_signal);

    // multiply by error_signal (chain rule)
    fetch::math::Multiply(error_signal, return_signal, return_signal);

    return {return_signal};
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "LogSigmoid";

private:
  // maximum possible output value of the log-sigmoid should not be zero, but actually epsilon
  DataType epsilon_ = DataType(1e-12);
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
