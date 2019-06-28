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
#include "math/ml/activation_functions/elu.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Elu : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  Elu(DataType a)
    : a_(a)
  {}

  virtual ~Elu() = default;

  void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 1);
    fetch::math::Elu(inputs.front().get(), a_, output);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs, ArrayType const &error_signal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == error_signal.shape());
    ArrayType ret{error_signal.shape()};

    DataType zero{0};
    DataType one{1};

    // gradient of elu function is a*e^x where x<0; and 1.0 where x>=0
    auto it  = inputs.front().get().cbegin();
    auto rit = ret.begin();
    while (it.is_valid())
    {
      if (*it >= zero)
      {
        // f(x)=x for x>=0
        *rit = one;
      }
      else
      {
        // f(x)=a*e^x
        fetch::math::Exp(*it, *rit);
        fetch::math::Multiply(a_, *rit, *rit);
      }
      ++it;
      ++rit;
    }

    // multiply by error_signal (chain rule)
    fetch::math::Multiply(error_signal, ret, ret);

    return {ret};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Elu";

private:
  DataType a_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
