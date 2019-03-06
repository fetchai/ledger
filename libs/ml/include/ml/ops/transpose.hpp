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
class Transpose : public fetch::ml::Ops<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  Transpose()          = default;
  virtual ~Transpose() = default;

  virtual ArrayPtrType Forward(std::vector<const ArrayPtrType> const &inputs)
  {
    ASSERT(inputs.size() == 1);

    this->output_ = std::make_shared<ArrayType>(inputs[0]);
    return this->output_->Transpose();
  }

  virtual std::vector<ArrayPtrType> Backward(std::vector<ArrayPtrType> const &inputs,
                                             ArrayPtrType                     errorSignal)
  {
    ASSERT(inputs.size() == 1);
    return {errorSignal->Clone().Transpose()};
  }

  static constexpr char const *DESCRIPTOR = "Transpose";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
