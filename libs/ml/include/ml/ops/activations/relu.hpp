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
class Relu : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  Relu()          = default;
  virtual ~Relu() = default;

  // f(x)=max(0,x);
  void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));
    fetch::math::Relu(inputs.front().get(), output);
  }

  /**
   * Gradients for backprop with Relu are as follows:
   * x>0 f'(x)=1, x<=0 f'(x)=0
   * therefore we should return error_signal but zeroed out at the relevant places
   * @param inputs
   * @param error_signal
   * @return
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs, ArrayType const &error_signal)
  {
    assert(inputs.size() == 1);
    assert(inputs.at(0).get().shape() == error_signal.shape());

    ArrayType const &input = inputs.front().get();
    ArrayType        return_signal{error_signal.shape()};

    auto it1    = input.begin();
    auto it2    = return_signal.begin();
    auto err_it = error_signal.cbegin();

    while (it1.is_valid())
    {
      if (*it1 <= static_cast<DataType>(0))
      {
        *it2 = static_cast<DataType>(0);
      }
      else
      {
        *it2 = *err_it;
      }
      ++it1;
      ++it2;
      ++err_it;
    }

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Relu";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
