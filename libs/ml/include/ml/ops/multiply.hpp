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

#include "math/free_functions/fundamental_operators.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Multiply : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Multiply()          = default;
  virtual ~Multiply() = default;

  /**
   * elementwise multiplication
   * @param inputs  left & right inputs to multiply
   * @return
   */
  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() > 1);
    for (std::size_t i = 1; i < inputs.size(); ++i)
    {
      assert(inputs[i]->shape() == inputs[i - 1]->shape());
    }

    std::vector<std::uint64_t> outputShape(inputs[0]->shape());
    if (!this->output_ || this->output_->shape() != outputShape)
    {
      this->output_ = std::make_shared<ArrayType>(outputShape);
    }

    fetch::math::Multiply(inputs[0], inputs[1], this->output_);
    if (inputs.size() > 2)
    {
      for (std::size_t i = 2; i < inputs.size(); ++i)
      {
        fetch::math::Multiply(this->output_, inputs[i], this->output_);
      }
    }

    return this->output_;
  }

  /**
   * elementwise multiplication is not trainable - just pass the error signal back
   */
  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    return std::vector<ArrayPtrType>(inputs.size(), errorSignal);
  }

  static constexpr char const *DESCRIPTOR = "Multiply";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
