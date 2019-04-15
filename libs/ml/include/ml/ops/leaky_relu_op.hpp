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
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/ml/activation_functions/leaky_relu.hpp"
#include "math/ml/activation_functions/relu.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LeakyReluOp : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  LeakyReluOp()          = default;
  virtual ~LeakyReluOp() = default;

  // f(x)=LeakyRelu(x,alpha)=max(0,x)+alpha*min(0,x)
  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(inputs.at(0).get().shape() == output.shape());
    ASSERT(inputs.at(1).get().size() == output.size());

    fetch::math::LeakyRelu(inputs.at(0).get(), inputs.at(1).get(), output);

    return output;
  }

  // input.at(0) - gradient for x: x>=0 f'(x)=1, x<0 f'(x)=alpha
  // input.at(1) - gradient for alpha: f'(alpha)=-Relu(-x)=min(0,x)
  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(inputs.at(0).get().size() == errorSignal.size());
    ASSERT(inputs.at(1).get().size() == errorSignal.size());
    ASSERT(errorSignal.size() == inputs.at(1).get().size());

    ArrayType returnSignal1{errorSignal.shape()};
    ArrayType returnSignal2{errorSignal.shape()};

    typename ArrayType::SizeType idx(0);
    for (auto const &val : inputs.at(0).get())
    {
      if (val >= DataType(0))
      {
        returnSignal1.Set(idx, DataType(1));
        returnSignal2.Set(idx, DataType(0));
      }
      else
      {
        returnSignal1.Set(idx, inputs.at(1).get().At(idx));
        returnSignal2.Set(idx, inputs.at(0).get().At(idx));
      }
      ++idx;
    }

    // f'(alpha)=-Relu(-x)
    // multiply by errorSignal (chain rule)
    fetch::math::Multiply(errorSignal, returnSignal1, returnSignal1);
    fetch::math::Multiply(errorSignal, returnSignal2, returnSignal2);

    // since PRelu is max(0,x)+alpha*min(0*x), signal for alpha is error*min(0*x)
    return {returnSignal1, returnSignal1};
  }

  static constexpr char const *DESCRIPTOR = "LeakyReluOp";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
