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
class Subtract : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType      = T;
  using DataType       = typename ArrayType::Type;
  using ArrayPtrType   = std::shared_ptr<ArrayType>;
  using ConstSliceType = typename ArrayType::ConstSliceType;
  Subtract()           = default;
  virtual ~Subtract()  = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    (void)output;
    ASSERT(inputs.size() == 2);
    ASSERT(inputs.at(0).get().size() == inputs.at(1).get().size());
    ASSERT(output.shape() == this->ComputeOutputShape(inputs));

    auto output_it  = output.begin();
    auto output_end = output.end();
    auto a_it       = inputs[0].get().begin();
    auto b_it       = inputs[1].get().begin();

    while (output_it != output_end)
    {
      *output_it = *a_it - *b_it;
      ++output_it;
      ++a_it;
      ++b_it;
    }

    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(inputs.at(0).get().size() == inputs.at(1).get().size());
    ASSERT(errorSignal.size() == inputs.at(1).get().size());

    return {errorSignal, fetch::math::Multiply(errorSignal, DataType{-1})};
  }

  static constexpr char const *DESCRIPTOR = "Subtract";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
