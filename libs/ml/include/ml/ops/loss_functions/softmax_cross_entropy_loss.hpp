#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
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

#include "math/activation_functions/sigmoid.hpp"
#include "math/activation_functions/softmax.hpp"
#include "math/fundamental_operators.hpp"
#include "math/metrics/cross_entropy.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class SoftmaxCrossEntropyLoss : public Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpSoftmaxCrossEntropySaveableParams<T>;
  using MyType        = SoftmaxCrossEntropyLoss<TensorType>;

  SoftmaxCrossEntropyLoss() = default;

  explicit SoftmaxCrossEntropyLoss(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~SoftmaxCrossEntropyLoss() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    return std::make_shared<SPType>(sp);
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

    return copyshare;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    // third term may be present for specifying n_classes
    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());

    // sanity check the softmax adds up to 1
    assert(static_cast<double>(Sum(fetch::math::Softmax((*inputs.at(0)))) -
                               (DataType(inputs.at(0)->shape().at(1)))) < 0.0001);

    // softmax forward & then CrossEntropy
    output(0, 0) =
        fetch::math::CrossEntropyLoss(fetch::math::Softmax((*inputs.at(0))), (*inputs.at(1)));
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(error_signal);

    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());

    TensorType ret({inputs.at(0)->shape()});
    fetch::math::Softmax((*inputs.at(0)), ret, 0);
    fetch::math::Subtract(ret, (*inputs.at(1)), ret);

    return {ret, ret};
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    (void)inputs;
    return {1, 1};
  }

  static constexpr OpType OpCode()
  {
    return OpType::LOSS_SOFTMAX_CROSS_ENTROPY;
  }
  static constexpr char const *DESCRIPTOR = "SoftmaxCrossEntropyLoss";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
