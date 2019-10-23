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
#include "ml/ops/ops.hpp"

#include <cassert>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class ReduceMean : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpReduceMeanSaveableParams<T>;
  using MyType        = ReduceMean<TensorType>;

  ReduceMean(SizeType axis)
    : axis_(axis)
  {}

  explicit ReduceMean(SPType const &sp)
    : Ops<T>(sp)
  {
    axis_ = sp.axis;
  }

  ~ReduceMean() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp  = std::make_shared<SPType>();
    sp->axis = axis_;
    return sp;
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
   * ReduceMean removes the first size 1 dimension encountered starting at the trailing edge.
   * If no dimensions are size 1, it will throw.
   * @param inputs vector containing one tensor which is the input tensor to ReduceMean
   * @return
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::ReduceMean((*inputs.at(0)), axis_, output);
  }

  /**
   * Just re-assign error to different shaped array:
   * f'(input0)= error_signal
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    TensorType ret_error_signal(this->ComputeOutputShape(inputs));

    Broadcast(
        [](DataType const &x, DataType const &y, DataType &z) {
          FETCH_UNUSED(y);
          z = x;
        },
        error_signal, error_signal, ret_error_signal);

    // Average error signal along specified axis
    fetch::math::Divide(ret_error_signal, static_cast<DataType>(inputs.at(0)->shape().at(axis_)),
                        ret_error_signal);

    return {ret_error_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    auto shape = inputs.front()->shape();

    shape.at(axis_) = static_cast<SizeType>(1);

    return shape;
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_REDUCE_MEAN;
  }

  SizeType axis_;

  static constexpr char const *DESCRIPTOR = "ReduceMean";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
