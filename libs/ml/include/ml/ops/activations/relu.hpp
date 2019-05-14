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
#include "math/ml/activation_functions/relu.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Relu : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType      = T;
  using DataType       = typename ArrayType::Type;
  using SizeType       = typename ArrayType::SizeType;
  using ArrayPtrType   = std::shared_ptr<ArrayType>;
  using ConstSliceType = typename ArrayType::ConstSliceType;

  Relu()          = default;
  virtual ~Relu() = default;

  // f(x)=max(0,x);
  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(output.shape() == this->ComputeOutputShape(inputs));
    fetch::math::Relu(inputs.front().get(), output);
    return output;
  }

  // x>0 f'(x)=1, x<=0 f'(x)=0
  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<const ArrayType>> const &inputs,
      ArrayType const &                                           error_signal)
  {
    ASSERT(inputs.size() == 1);
    ASSERT(inputs[0].get().shape() == error_signal.shape());

    ArrayType const &input = inputs.front().get();
    ArrayType        return_signal{error_signal.shape()};

    auto it1 = input.begin();
    auto it2 = return_signal.begin();

    while (it1.is_valid())
    {
      if (*it1 <= DataType(0))
      {
        *it2 = DataType(0);
      }
      ++it1;
      ++it2;
    }

    return {return_signal};
  }

  static constexpr char const *DESCRIPTOR = "Relu";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
