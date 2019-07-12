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
class TanH : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  TanH()          = default;
  virtual ~TanH() = default;

  std::shared_ptr<SaveableParams<ArrayType>> GetOpSaveableParams()
  {
    SaveableParams<ArrayType> sp{};
    sp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<SaveableParams<ArrayType>>(sp);
  }

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));
    fetch::math::TanH(inputs.front().get(), output);
    // ensures numerical stability
    for (auto &val : output)
    {
      // Minimum value of tanh is restricted to -1+epsilon
      fetch::math::Max(val, fetch::math::Add(DataType(-1), epsilon_), val);
      // Maximum value of tanh is restricted to 1-epsilon
      fetch::math::Min(val, fetch::math::Subtract(static_cast<DataType>(1), epsilon_), val);
    }
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    assert(inputs.size() == 1);

    assert(inputs.front().get().shape() == error_signal.shape());

    ArrayType return_signal = error_signal.Copy();

    ArrayType t(this->ComputeOutputShape(inputs));
    Forward(inputs, t);

    // gradient of tanh: 1 - tanh(x)^2
    fetch::math::Multiply(t, t, t);
    fetch::math::Subtract(static_cast<DataType>(1), t, t);

    // apply chain rule
    fetch::math::Multiply(error_signal, t, return_signal);

    return {return_signal};
  }

  virtual std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return inputs.front().get().shape();
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
