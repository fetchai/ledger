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

#include "math/activation_functions/softmax.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Softmax : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpSoftmaxSaveableParams<T>;

  explicit Softmax(SizeType axis = 0)
    : axis_(axis)
  {}

  explicit Softmax(std::vector<SizeType> axes)
    : axes_(axes)
  {}

  explicit Softmax(SPType const &sp)
  {
    axis_ = sp.axis;
    axes_ = sp.axes;
  }

  ~Softmax() override = default;

  std::shared_ptr<SaveableParamsInterface> GetOpSaveableParams() override
  {
    auto sp_ptr  = std::make_shared<SPType>();
    sp_ptr->axis = axis_;
    sp_ptr->axes = axes_;
    return sp_ptr;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(output.shape() == ComputeOutputShape(inputs));
    assert(inputs.size() == 1);

    if (axes_.size() == 0)
    {
      fetch::math::Softmax((*inputs.at(0)), output, axis_);
    }
    else
    {
      fetch::math::Softmax((*inputs.at(0)), output, axes_);
    }
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front()->shape() == error_signal.shape());

    TensorType return_signal = error_signal.Copy();
    TensorType t(error_signal.shape());
    this->Forward(inputs, t);

    fetch::math::Multiply(return_signal, t, return_signal);

    // 1D softmax with 1 batch dimension
    if (inputs.front()->shape().size() == 1)
    {
      typename TensorType::Type sum = return_signal.Sum();
      fetch::math::Multiply(t, sum, t);
    }
    // N-D softmax
    else
    {
      if (axes_.size() == 0)
      {
        TensorType sum = ReduceSum(return_signal, axis_);
        fetch::math::Multiply(t, sum, t);
      }
      else
      {
        TensorType sum = ReduceSum(return_signal, axes_);
        fetch::math::Multiply(t, sum, t);
      }
    }

    fetch::math::Subtract(return_signal, t, return_signal);

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_SOFTMAX;
  }
  static constexpr char const *DESCRIPTOR = "Softmax";

private:
  SizeType              axis_;
  std::vector<SizeType> axes_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
