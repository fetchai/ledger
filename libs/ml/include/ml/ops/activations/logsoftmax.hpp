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

#include "core/macros.hpp"
#include "math/activation_functions/softmax.hpp"
#include "math/fundamental_operators.hpp"
#include "math/standard_functions/log.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LogSoftmax : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  explicit LogSoftmax(SizeType axis = 0)
    : axis_(axis)
  {}
  ~LogSoftmax() override = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(output.shape() == ComputeOutputShape(inputs));
    assert(inputs.size() == 1);
    fetch::math::Softmax((*inputs.front()), output, axis_);
    fetch::math::Log(output, output);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front()->shape() == error_signal.shape());

    ArrayType return_signal = error_signal.Copy();
    ArrayType t(error_signal.shape());
    fetch::math::Softmax((*inputs.front()), t, axis_);

    // return_signal.InlineMultiply(t);

    // 1D softmax with 1 batch dimension
    if (inputs.front()->shape().size() == 2)
    {
      assert(axis_ == 1);
      ArrayType sum = ReduceSum(return_signal, 0);

      t.InlineMultiply(sum);
    }
    // 2D softmax with 1 batch dimension
    else if (inputs.front()->shape().size() == 3)
    {
      assert((axis_ == 1) || (axis_ == 0));
      ArrayType sum = return_signal.Slice(0, 1 - axis_).Copy();
      for (size_t i = 0; i < return_signal.shape()[2]; i++)
      {
        sum.View(i).Assign(ReduceSum(return_signal.Slice(i, 2).Copy().Squeeze(), 1 - axis_).View());
      }

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

  static constexpr char const *DESCRIPTOR = "LogSoftmax";

private:
  SizeType axis_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
