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

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Concatenate : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = fetch::math::SizeType;

  Concatenate(SizeType axis)
    : axis_(axis)
  {}

  virtual ~Concatenate() = default;

  /**
   * concatenates multiple input tensors into one
   */
  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    std::vector<ArrayType> tensors;
    for (std::size_t i = 0; i < inputs.size(); ++i)
    {
      tensors.emplace_back(inputs[i].get());
    }

    output = ArrayType::Concat(tensors, axis_);
    return output;
  }

  /**
   * Splits up the gradients to feed back to the inputs
   * @param inputs
   * @param error_signal
   * @return
   */
  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           error_signal)
  {
    concat_points_.resize(inputs.size());
    for (SizeType i{0}; i < inputs.size(); ++i)
    {
      concat_points_[i] = inputs[i].get().shape()[axis_];
    }
    return ArrayType::Split(error_signal, concat_points_, axis_);
  }

  static constexpr char const *DESCRIPTOR = "Concatenate";

private:
  SizeType              axis_;
  std::vector<SizeType> concat_points_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
