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

#include "core/macros.hpp"
#include "math/activation_functions/softmax.hpp"
#include "math/fundamental_operators.hpp"
#include "math/standard_functions/log.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class LogSoftmax : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpLogSoftmaxSaveableParams<T>;
  using MyType        = LogSoftmax<TensorType>;

  explicit LogSoftmax(SizeType axis = 1)
    : axis_(axis)
  {}

  explicit LogSoftmax(SPType const &sp)
    : Ops<T>(sp)
  {
    axis_ = sp.axis;
  }

  ~LogSoftmax() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp_ptr  = std::make_shared<SPType>();
    sp_ptr->axis = axis_;
    return sp_ptr;
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
    assert(output.shape() == ComputeOutputShape(inputs));
    assert(inputs.size() == 1);
    fetch::math::Softmax((*inputs.front()), output, axis_);
    fetch::math::Log(output, output);
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front()->shape() == error_signal.shape());

    TensorType return_signal = error_signal.Copy();
    TensorType t(error_signal.shape());
    fetch::math::Softmax((*inputs.front()), t, axis_);

    // return_signal.InlineMultiply(t);

    // N-D softmax with 1 batch dimension
    if (inputs.front()->shape().size() > 1)
    {
      TensorType sum = ReduceSum(return_signal, axis_);
      t.InlineMultiply(sum);
    }

    return_signal.InlineSubtract(t);
    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_LOGSOFTMAX;
  }
  static constexpr char const *DESCRIPTOR = "LogSoftmax";

private:
  SizeType axis_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
