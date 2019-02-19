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

#include "core/assert.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Add : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Add()          = default;
  virtual ~Add() = default;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(inputs[0]->size() == inputs[1]->size());
    if (!this->output_ || this->output_->shape() != inputs[0]->shape())
    {
      this->output_ = std::make_shared<ArrayType>(inputs[0]->shape());
    }

    for (std::uint64_t i = 0; i < inputs[0]->size(); ++i)
    {
      this->output_->Set(i, inputs[0]->At(i) + inputs[1]->At(i));
    }
    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(inputs[0]->size() == inputs[1]->size());
    ASSERT(errorSignal->size() == inputs[1]->size());
    return {errorSignal, errorSignal};
  }

private:
  // Relu is done in a strange way, comparing input against an array of zeroes
  // using a parrallel Maximum function -- May need improvement (TODO private 469)
  ArrayPtrType zeroes_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
