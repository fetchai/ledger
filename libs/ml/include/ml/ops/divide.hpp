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
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Divide : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  Divide()          = default;
  virtual ~Divide() = default;

  /**
   * elementwise multiplication
   * @param inputs  left & right inputs to Divide
   * @return
   */
  virtual void Forward(std::vector<ArrayPtrType> const &inputs, ArrayType &output)
  {
    (void)output;
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

    fetch::math::Divide(inputs[0], inputs[1], this->output_);
    if (inputs.size() > 2)
    {
      for (std::size_t i = 2; i < inputs.size(); ++i)
      {
        fetch::math::Divide(this->output_, inputs[i], this->output_);
      }
    }

    output = this->output_;
  }

  /**
   * elementwise multiplication is not trainable - just pass the error signal back
   */
  virtual std::vector<ArrayPtrType> Backward(VecTensorType const &inputs, ArrayPtrType error_signal)
  {
    return std::vector<ArrayPtrType>(inputs.size(), error_signal);
  }

  static constexpr char const *DESCRIPTOR = "Divide";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
