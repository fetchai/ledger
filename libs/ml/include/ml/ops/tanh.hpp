#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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
#include "vectorise/math/max.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class TanH : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpTanhSaveableParams<T>;
  using MyType        = TanH<TensorType>;

  TanH() = default;

  explicit TanH(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~TanH() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    return std::make_shared<SPType>(sp);
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

    return copyshare;
  }
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));
    fetch::math::TanH(*(inputs.front()), output);
    // ensures numerical stability
    for (auto &val : output)
    {
      // Minimum value of tanh is restricted to -1+epsilon
      val = fetch::vectorise::Max(val, fetch::math::Add(DataType(-1), epsilon_));
      // Maximum value of tanh is restricted to 1-epsilon
      val = fetch::vectorise::Min(val, fetch::math::Subtract(DataType{1}, epsilon_));
    }
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);

    assert(inputs.front()->shape() == error_signal.shape());

    TensorType return_signal = error_signal.Copy();

    TensorType t(this->ComputeOutputShape(inputs));
    Forward(inputs, t);

    // gradient of tanh: 1 - tanh(x)^2
    fetch::math::Multiply(t, t, t);
    fetch::math::Subtract(DataType{1}, t, t);

    // apply chain rule
    fetch::math::Multiply(error_signal, t, return_signal);

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_TANH;
  }

  static constexpr char const *DESCRIPTOR = "TanH";

private:
  // minimum possible output value of the tanh should not be -1, but actually (-1 + epsilon)
  // likewise maximum output should be (1 - epsilon)
  DataType epsilon_ = fetch::math::numeric_min<DataType>();
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
