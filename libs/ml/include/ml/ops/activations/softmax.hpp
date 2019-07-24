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

  explicit Softmax(SizeType axis = 1)
    : axis_(axis)
  {}
  ~Softmax() override = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(output.shape() == ComputeOutputShape(inputs));
    assert(inputs.size() == 1);
    fetch::math::Softmax(inputs.at(0).get(), output, axis_);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == error_signal.shape());

    ArrayType return_signal = error_signal.Copy();
    ArrayType t(error_signal.shape());
    this->Forward(inputs, t);
    return_signal.InlineMultiply(t);

    // 1D softmax with 1 batch dimension
    if (inputs.front().get().shape().size() == 2)
    {
      assert(axis_ == 1);
      ArrayType sum = ReduceSum(return_signal, 0);

      t.InlineMultiply(sum);
    }
    // 2D softmax with 1 batch dimension
    else if (inputs.front().get().shape().size() == 3)
    {
      assert((axis_ == 1) || (axis_ == 0));
      ArrayType sum = return_signal.Slice(0, 1 - axis_).Copy();
      for (size_t i = 0; i < return_signal.shape()[2]; i++)
      {
        auto cur_sum = ReduceSum(return_signal.Slice(i, 2).Copy().Squeeze(), 1 - axis_).View();
        sum.View(i).Assign(cur_sum);
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
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Softmax";

private:
  SizeType axis_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
