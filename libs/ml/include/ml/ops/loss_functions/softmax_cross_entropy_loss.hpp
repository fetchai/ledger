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

#include "math/activation_functions/sigmoid.hpp"
#include "math/activation_functions/softmax.hpp"
#include "math/fundamental_operators.hpp"
#include "math/metrics/cross_entropy.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class SoftmaxCrossEntropyLoss : public Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  SoftmaxCrossEntropyLoss()          = default;
  virtual ~SoftmaxCrossEntropyLoss() = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    // third term may be present for specifying n_classes
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().size() == inputs.at(1).get().size());

    // sanity check the softmax adds up to 1
    assert(Sum(fetch::math::Softmax(inputs.at(0).get())) - (DataType(inputs.at(0)->shape().at(0))) <
           0.0001);

    // softmax forward & then CrossEntropy
    output(0, 0) =
        fetch::math::CrossEntropyLoss(fetch::math::Softmax(inputs.at(0).get()), inputs.at(1).get());
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    FETCH_UNUSED(error_signal);

    assert(inputs.size() == 2);
    assert(inputs.at(0).get().size() == inputs.at(1).get().size());

    ArrayType ret({inputs.at(0)->shape()});
    fetch::math::Softmax(inputs.at(0).get(), ret);
    fetch::math::Subtract(ret, inputs.at(1).get(), ret);

    return {ret, ret};
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    (void)inputs;
    return {1, 1};
  }

  static constexpr char const *DESCRIPTOR = "SoftmaxCrossEntropyLoss";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
