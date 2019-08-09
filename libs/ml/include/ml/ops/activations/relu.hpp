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
#include "math/activation_functions/relu.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Relu : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpReluSaveableParams<T>;

  Relu() = default;

  explicit Relu(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Relu() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    return std::make_shared<SPType>();
  }

  // f(x)=max(0,x);
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));
    fetch::math::Relu((*inputs.front()), output);
  }

  /**
   * Gradients for backprop with Relu are as follows:
   * x>0 f'(x)=1, x<=0 f'(x)=0
   * therefore we should return error_signal but zeroed out at the relevant places
   * @param inputs
   * @param error_signal
   * @return
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.at(0)->shape() == error_signal.shape());

    TensorType const &input = (*inputs.front());
    TensorType        return_signal{error_signal.shape()};

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

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_RELU;
  }
  static constexpr char const *DESCRIPTOR = "Relu";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
