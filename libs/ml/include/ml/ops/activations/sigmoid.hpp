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
#include "math/free_functions/ml/activation_functions/sigmoid.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Sigmoid : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Sigmoid()          = default;
  virtual ~Sigmoid() = default;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 1);
    if (!this->output_ || this->output_->shape() != inputs[0]->shape())
    {
      this->output_ = std::make_shared<ArrayType>(inputs[0]->shape());
    }

    fetch::math::Sigmoid(*inputs[0], *this->output_);
    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs[0]->shape() == errorSignal->shape());

    ArrayPtrType t = this->Forward(inputs);
    errorSignal *= fetch::math::Multiply(*t, fetch::math::Subtract(1, *t));
    return {errorSignal};
  }

  static std::string Descriptor()
  {
    return "Sigmoid";
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
