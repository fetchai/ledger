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

#include <cassert>
#include <memory>
#include <vector>

#include "math/fundamental_operators.hpp"
#include "math/ml/activation_functions/sigmoid.hpp"
#include "math/ml/activation_functions/softmax.hpp"
#include "math/ml/loss_functions/cross_entropy.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class SoftmaxCrossEntropyOp : public Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;

  SoftmaxCrossEntropyOp()          = default;
  virtual ~SoftmaxCrossEntropyOp() = default;

  virtual void Forward(VecTensorType const &inputs, ArrayType &output)
  {
    // third term may be present for specifying n_classes
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().size() == inputs.at(1).get().size());

    // sanity check the softmax adds up to 1
    assert(Sum(fetch::math::Softmax(inputs.at(0).get())) -
               (DataType(inputs.at(0).get().shape().at(0))) <
           0.0001);

    // softmax forward & then CrossEntropy
    output(0, 0) =
        fetch::math::CrossEntropyLoss(fetch::math::Softmax(inputs[0].get()), inputs[1].get());
  }

  virtual std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                          ArrayType const &    error_signal)
  {
    assert(inputs.size() == 2);
    assert(inputs[0].get().size() == inputs[1].get().size());

    ArrayType ret = fetch::math::Subtract(fetch::math::Softmax(inputs[0].get()), inputs[1].get());

    // chain rule
    fetch::math::Multiply(ret, error_signal, ret);

    return {ret, ret};
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const
  {
    (void)inputs;
    return {1, 1};
  }

  static constexpr char const *DESCRIPTOR = "SoftmaxCrossEntropyOp";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
