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
class Multiply : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType     = T;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename ElementWiseOps<T>::VecTensorType;

  Multiply()          = default;
  virtual ~Multiply() = default;

  /**
   * elementwise multiplication
   * @param inputs  left & right inputs to multiply
   * @return
   */
  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(inputs.at(0).get().size() == inputs.at(1).get().size());
    ASSERT(output.shape() == this->ComputeOutputShape(inputs));

    auto output_it  = output.begin();
    auto output_end = output.end();
    auto a_it       = inputs[0].get().begin();
    auto b_it       = inputs[1].get().begin();

    while (output_it != output_end)
    {
      *output_it = *a_it * *b_it;
      ++output_it;
      ++a_it;
      ++b_it;
    }
  }

  /**
   * elementwise multiplication gradient is:
   * f'(input0)=input0*error_signal
   * f'(input1)=input1*error_signal
   */
  virtual std::vector<ArrayPtrType> Backward(VecTensorType const &inputs, ArrayPtrType error_signal)
  {
    ASSERT(inputs.size() == 2);
    ASSERT(inputs.at(0).get().size() == inputs.at(1).get().size());
    ASSERT(error_signal.size() == inputs.at(1).get().size());

    ArrayType error_signal1(inputs.at(0).get().shape());
    ArrayType error_signal2(inputs.at(1).get().shape());

    fetch::math::Multiply(inputs.at(1).get(), error_signal, error_signal1);
    fetch::math::Multiply(inputs.at(0).get(), error_signal, error_signal2);

    return {error_signal1, error_signal2};
  }

  static constexpr char const *DESCRIPTOR = "Multiply";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
