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
#include "math/free_functions/trigonometry/trigonometry.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Tanh : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Tanh()          = default;
  virtual ~Tanh() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    assert(inputs.size() == 1);
    if (!this->output_ || this->output_->shape() != inputs.front().get().shape())
    {
      this->output_ = std::make_shared<ArrayType>(inputs.front().get().shape());
    }

    fetch::math::Tanh(inputs.front().get(), *this->output_);
    return *(this->output_);
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 1);

    assert(inputs.front().get().shape() == errorSignal.shape());

    ArrayType t            = this->Forward(inputs);
    ArrayType returnSignal = errorSignal.Clone();
    fetch::math::Multiply(
        errorSignal, fetch::math::Subtract(DataType(1), fetch::math::Multiply(t, t)), returnSignal);
    return {returnSignal};
  }

  static constexpr char const *DESCRIPTOR = "Tanh";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
