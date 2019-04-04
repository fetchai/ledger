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

#include "math/ml/activation_functions/relu.hpp"

#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Relu : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Relu()          = default;
  virtual ~Relu() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    (void)output;
    assert(inputs.size() == 1);
    if (!this->output_ || this->output_->shape() != inputs.front().get().shape())
    {
      this->output_ = std::make_shared<ArrayType>(inputs.front().get().shape());
    }

    fetch::math::Relu(inputs.front().get(), *this->output_);
    return *(this->output_);
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs[0].get().shape() == errorSignal.shape());

    ArrayType returnSignal = errorSignal.Clone();
    for (std::size_t i = 0; i < inputs.front().get().size(); ++i)
    {
      if (inputs.front().get()[i] <= DataType(0))
      {
        returnSignal.Set(i, DataType(0));
      }
    }
    return {returnSignal};
  }

  static constexpr char const *DESCRIPTOR = "Relu";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
