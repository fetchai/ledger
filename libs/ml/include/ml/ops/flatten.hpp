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
class Flatten : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SliceType    = typename ArrayType::SliceType;

  Flatten()          = default;
  virtual ~Flatten() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<SliceType const>> const &inputs)
  {
    ASSERT(inputs.size() == 1);
    input_shape_ = inputs.front().get().shape();
    this->output_ =
        std::make_shared<ArrayType>(std::vector<std::uint64_t>({1, inputs.front().get().size()}));
    this->output_->Copy(inputs.front().get());
    return *this->output_;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<SliceType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == 1);
    ArrayType ret(input_shape_);
    ret.Copy(errorSignal);
    return {ret};
  }

  static constexpr char const *DESCRIPTOR = "Flatten";

private:
  std::vector<std::uint64_t> input_shape_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
