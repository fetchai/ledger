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

#include "math/activation_functions/sigmoid.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/standard_functions/clamp.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Sigmoid : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = SaveableParams;

  Sigmoid() = default;

  explicit Sigmoid(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Sigmoid() override = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams()
  {
    SPType sp{};
    sp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<SPType>(sp);
  }

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));
    fetch::math::Sigmoid(inputs.front().get(), output);

    // ensures numerical stability
    fetch::math::Clamp(epsilon_, static_cast<DataType>(1) - epsilon_, output);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == error_signal.shape());
    ArrayType return_signal{error_signal.shape()};
    ArrayType t{inputs.front().get().shape()};

    // gradient of sigmoid function is s(x)(1 - s(x))
    Forward(inputs, t);
    fetch::math::Subtract(static_cast<DataType>(1), t, return_signal);
    fetch::math::Multiply(t, return_signal, return_signal);

    // multiply by error_signal (chain rule)
    fetch::math::Multiply(error_signal, return_signal, return_signal);

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Sigmoid";

private:
  // minimum possible output value of the sigmoid should not be zero, but actually epsilon
  // likewise maximum output should be 1 - epsilon
  DataType epsilon_ = DataType(1e-7);
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
