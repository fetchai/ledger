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
class Flatten : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Flatten()          = default;
  virtual ~Flatten() = default;

  virtual ArrayPtrType Forward(std::vector<ArrayPtrType> const &inputs)
  {
    ASSERT(inputs.size() == 1);
    input_shape_  = inputs[0]->shape();
    this->output_ = std::make_shared<ArrayType>(std::vector<size_t>({1, inputs[0]->size()}));
    // TODO(private, 521) remove useless copy and replace with lightweight view
    this->output_->Copy(*inputs[0]);
    return this->output_;
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    ASSERT(inputs.size() == 1);
    std::shared_ptr<ArrayType> ret = std::make_shared<ArrayType>(input_shape_[0]);
    ret->Copy(*errorSignal);
    return {ret};
  }

private:
  std::vector<size_t> input_shape_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
