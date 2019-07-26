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
class Softmax : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = SoftmaxSaveableParams;

  explicit Softmax(SizeType axis = 1)
    : axis_(axis)
  {}

  explicit Softmax(SPType const &sp)
  {
    axis_ = sp.axis;
  }

  ~Softmax() override = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams() override
  {
    auto sp_ptr        = std::make_shared<SPType>();
    sp_ptr->DESCRIPTOR = DESCRIPTOR;
    sp_ptr->axis       = axis_;
    return sp_ptr;
  }

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(output.shape() == ComputeOutputShape(inputs));
    assert(inputs.size() == 1);
    fetch::math::Softmax((*inputs.at(0)), output, axis_);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front()->shape() == error_signal.shape());

    ArrayType return_signal = error_signal.Copy();
    ArrayType t(error_signal.shape());
    this->Forward(inputs, t);
    return_signal.InlineMultiply(t);

    // 1D softmax
    if (inputs.front()->shape().size() == 1)
    {
      typename ArrayType::Type sum = return_signal.Sum();
      t.InlineMultiply(sum);
    }
    // 2D softmax
    else if (inputs.front()->shape().size() == 2)
    {
      ArrayType sum;
      sum = ReduceSum(return_signal, 1 - axis_);

      t.InlineMultiply(sum);
    }
    else
    {
      throw std::runtime_error("Softmax over >= 3 dimensions not implemented");
    }

    return_signal.InlineSubtract(t);
    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr char const *DESCRIPTOR = "Softmax";

private:
  SizeType axis_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
