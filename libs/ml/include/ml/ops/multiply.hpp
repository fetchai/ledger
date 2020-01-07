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

#include "core/assert.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Multiply : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpMultiplySaveableParams<T>;
  using MyType        = Multiply<TensorType>;

  Multiply() = default;

  explicit Multiply(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Multiply() override = default;

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
  /**
   * elementwise multiplication
   * for inputs to the multiply layer, if broadcasting is required, make sure the first input is the
   * one with the complete shape
   *
   * @param inputs  left & right inputs to multiply
   * @return
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->shape().size() <=
           3);  // we do not support input of more than 3D (including batch dims)
    assert(inputs.at(0)->shape().size() ==
           inputs.at(1)->shape().size());  // check if addition is broadcastable
    assert(output.shape() == inputs.front()->shape());

    fetch::math::Multiply((*inputs.at(0)), (*inputs.at(1)), output);
  }

  /**
   * elementwise multiplication gradient is:
   * f'(input0)=input1*error_signal
   * f'(input1)=input0*error_signal
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->shape().size() <=
           3);  // we do not support input of more than 3D (including batch dims)
    assert(inputs.at(0)->shape().size() ==
           inputs.at(1)->shape().size());  // check if addition is broadcastable
    assert(error_signal.shape() == inputs.front()->shape());

    TensorType error_signal_1(error_signal.shape());
    TensorType error_signal_2(error_signal.shape());
    fetch::math::Multiply(error_signal, (*inputs.at(1)), error_signal_1);
    fetch::math::Multiply(error_signal, (*inputs.at(0)), error_signal_2);

    if (inputs.at(0)->shape() == inputs.at(1)->shape())
    {
      return {error_signal_1, error_signal_2};
    }
    if (inputs.at(1)->size() == 1)
    {
      // if second input is a scalar
      auto second_error_signal = TensorType(inputs.at(1)->shape());
      fetch::math::Sum(error_signal_2, *second_error_signal.begin());
      return {error_signal_1, second_error_signal};
    }

    // since the shape is not compatible, then the second input must have size 1 in batch dims
    SizeType batch_dimension = inputs.at(0)->shape().size() - 1;
    assert(inputs.at(1)->shape().at(batch_dimension) == 1);
    if (inputs.at(1)->shape().size() == 2)
    {
      // NB * N1 case
      return {error_signal_1, fetch::math::ReduceSum(error_signal_2, batch_dimension)};
    }

    // in the case where we have three dims
    // We only support backward broadcast through shape (N, 1, 1)
    assert(inputs.at(1)->shape(1) == 1);

    TensorType error_sum({inputs.at(1)->shape(0), 1});
    for (SizeType batch = 0; batch < error_signal.shape(batch_dimension); batch++)
    {
      error_sum += fetch::math::ReduceSum(error_signal_2.View(batch).Copy(), SizeType(1));
    }
    error_sum.Reshape(inputs.at(1)->shape());
    return {error_signal_1, error_sum};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_MULTIPLY;
  }
  static constexpr char const *DESCRIPTOR = "Multiply";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
