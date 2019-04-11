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

#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/ml/activation_functions/leaky_relu.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LeakyRelu : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;

  LeakyRelu(DataType a = DataType(0.01))
    : a_(a)
  {}

  virtual ~LeakyRelu() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    assert(inputs.size() == 1);
    fetch::math::LeakyRelu(inputs.front().get(), a_, output);
    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == errorSignal.shape());
    ArrayType returnSignal{errorSignal.shape()};
    ArrayType t{inputs.front().get().shape()};

    // gradient of parametric relu function is for x<0 = a, x>=0 = 1.0
    t = this->Forward(inputs, t);

    typename ArrayType::SizeType idx(0);
    for (auto const &val : t)
    {
      if (val >= DataType(0))
      {
        returnSignal.Set(idx, DataType(1));
      }
      else
      {
        returnSignal.Set(idx, a_);
      }
      ++idx;
    }

    // multiply by errorSignal (chain rule)
    fetch::math::Multiply(errorSignal, returnSignal, returnSignal);

    return {returnSignal};
  }

  static constexpr char const *DESCRIPTOR = "LeakyRelu";

private:
  DataType a_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
