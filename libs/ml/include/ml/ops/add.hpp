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

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(inputs.at(0).get().size() == inputs.at(1).get().size());
    if (!this->output_ || this->output_->shape() != inputs.at(0).get().shape())
    {
      this->output_ = std::make_shared<ArrayType>(inputs.at(0).get().shape());
    }

    this->output_->Fill(DataType(0));
    this->output_->InlineAdd(inputs[0]);
    this->output_->InlineAdd(inputs[1]);
    return *this->output_;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(inputs.at(0).get().size() == inputs.at(1).get().size());
    ASSERT(errorSignal.size() == inputs.at(1).get().size());
    return {errorSignal, errorSignal};
  }

  static constexpr char const *DESCRIPTOR = "Add";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
