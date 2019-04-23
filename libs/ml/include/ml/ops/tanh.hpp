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
#include "math/trigonometry.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class TanH : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType = T;
  using DataType  = typename ArrayType::Type;

  TanH()          = default;
  virtual ~TanH() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    assert(inputs.size() == 1);
    if (!this->output_ || this->output_->shape() != inputs.front().get().shape())
    {
      this->output_ = std::make_shared<ArrayType>(inputs.front().get().shape());
    }

    fetch::math::TanH(inputs.front().get(), *this->output_);

    // ensures numerical stability
    for (auto &val : *this->output_)
    {
      // Minimum value of tanh is restricted to -1+epsilon
      fetch::math::Max(val, fetch::math::Add(DataType(-1), epsilon_), val);
      // Maximum value of tanh is restricted to 1-epsilon
      fetch::math::Min(val, fetch::math::Subtract(DataType(1), epsilon_), val);
    }

    return *(this->output_);
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           error_signal)
  {
    assert(inputs.size() == 1);

    assert(inputs.front().get().shape() == error_signal.shape());

    ArrayType return_signal = error_signal.Clone();
    ArrayType t             = this->Forward(inputs);

    // gradient of tanh: 1 - tanh(x)^2
    fetch::math::Multiply(t, t, t);
    fetch::math::Subtract(DataType(1), t, t);

    // apply chain rule
    fetch::math::Multiply(error_signal, t, return_signal);

    return {return_signal};
  }

  static constexpr char const *DESCRIPTOR = "TanH";

private:
  // minimum possible output value of the tanh should not be -1, but actually (-1 + epsilon)
  // likewise maximum output should be (1 - epsilon)
  DataType epsilon_ = DataType(1e-12);
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
