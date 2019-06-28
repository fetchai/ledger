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
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Divide : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SizeType      = typename ArrayType::SizeType;

  Divide()          = default;
  virtual ~Divide() = default;

  /**
   * elementwise division
   * @param inputs  left & right inputs to Divide
   * @return
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(output.shape() == this->ComputeOutputShape(inputs));

    assert(inputs.size() > 1);
    for (std::size_t i = 1; i < inputs.size(); ++i)
    {
      assert(inputs[i].get().shape() == inputs[i - 1].get().shape());
    }

    fetch::math::Divide(inputs[0].get(), inputs[1].get(), output);
    if (inputs.size() > 2)
    {
      for (std::size_t i = 2; i < inputs.size(); ++i)
      {
        fetch::math::Divide(output, inputs[i].get(), output);
      }
    }
  }

  /**
   * elementwise division is not trainable - just pass the error signal back
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs, ArrayType const &  error_signal) override
  {
    return std::vector<ArrayType>(inputs.size(), error_signal);
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Divide";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
