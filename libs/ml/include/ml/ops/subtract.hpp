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

#include "core/macros.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Subtract : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  Subtract()           = default;
  ~Subtract() override = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().size() == inputs.at(1).get().size());
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::Subtract(inputs[0].get(), inputs[1].get(), output);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().size() == inputs.at(1).get().size());
    assert(error_signal.size() == inputs.at(1).get().size());

    return {error_signal, fetch::math::Multiply(error_signal, DataType{-1})};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Subtract";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
