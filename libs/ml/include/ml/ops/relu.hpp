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

#include "math/free_functions/free_functions.hpp"
#include "ml/ops/derivatives/derivatives.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class ReluLayer : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  ReluLayer()          = default;
  virtual ~ReluLayer() = default;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    assert(inputs.size() == 1);

    if (!zeroes_ || zeroes_->shape() != inputs[0]->shape())
    {
      zeroes_ = std::make_shared<ArrayType>(inputs[0]->shape());
    }

    if (!this->output_ || this->output_->shape() != inputs[0]->shape())
    {
      this->output_ = std::make_shared<ArrayType>(inputs[0]->shape());
    }

    fetch::math::Maximum(*(inputs[0]), *zeroes_, *this->output_);

    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs[0]->shape() == errorSignal->shape());

    for (std::size_t i = 0; i < inputs[0]->size(); ++i)
    {
      if ((*(inputs[0]))[i] <= 0)
      {
        errorSignal->Set(i, 0);
      }
    }
    return {errorSignal};
  }

private:
  // Relu is done in a strange way, comparing input against an array of zeroes
  // using a parrallel Maximum function -- May need improvement (TODO private 469)
  ArrayPtrType zeroes_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
