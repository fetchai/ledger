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

#include "ml/ops/ops.hpp"

#include <functional>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Concatenate : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = SoftmaxSaveableParams;

  explicit Concatenate(SizeType axis)
    : axis_(axis)
  {}

  explicit Concatenate(SPType const &sp)
  {
    axis_ = sp.axis;
  }

  ~Concatenate() override = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams() override
  {
    auto sp_ptr        = std::make_shared<SPType>();
    sp_ptr->DESCRIPTOR = DESCRIPTOR;
    sp_ptr->axis       = axis_;
    return sp_ptr;
  }

  /**
   * concatenates multiple input tensors into one
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    std::vector<ArrayType> tensors;
    for (auto const &e : inputs)
    {
      tensors.emplace_back(*e);
    }

    output = ArrayType::Concat(tensors, axis_);
  }

  /**
   * Splits up the gradients to feed back to the inputs
   * @param inputs
   * @param error_signal
   * @return
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    concat_points_.resize(inputs.size());
    auto c_it = concat_points_.begin();
    for (auto const &e : inputs)
    {
      *c_it = e->shape()[axis_];
      ++c_it;
    }

    return ArrayType::Split(error_signal, concat_points_, axis_);
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr char const *DESCRIPTOR = "Concatenate";

private:
  SizeType              axis_;
  std::vector<SizeType> concat_points_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
