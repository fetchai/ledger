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

#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Slice : public Ops<T>
{
public:
  using TensorType     = T;
  using SizeType       = typename TensorType::SizeType;
  using ArrayPtrType   = std::shared_ptr<TensorType>;
  using VecTensorType  = typename Ops<T>::VecTensorType;
  using ConstSliceType = typename TensorType::ConstSliceType;
  using SPType         = OpSliceSaveableParams<T>;
  using MyType         = Slice<TensorType>;

  explicit Slice(std::vector<SizeType> indices, std::vector<SizeType> axes)
  {
    indices_ = indices;
    axes_    = axes;
  }
  explicit Slice(SizeType index, SizeType axis)
  {
    index_ = index;
    axis_  = axis;
  }
  explicit Slice(SPType const &sp)
    : Ops<T>(sp)
  {
    indices_ = sp.indices;
    axes_    = sp.axes;
    index_   = sp.index;
    axis_    = sp.axis;
  }

  ~Slice() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp = std::make_shared<SPType>();

    sp->indices = indices_;
    sp->axes    = axes_;
    sp->index   = index_;
    sp->axis    = axis_;

    return sp;
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    return std::make_shared<MyType>(*this);  // calls default copy constructor of MyType;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    if (axes_.empty())
    {
      output.Assign(inputs.front()->Slice(index_, axis_));
    }
    else
    {
      output.Assign(inputs.front()->Slice(indices_, axes_));
    }
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    // N.B. At this position of the code, we have to make sure that every position other than the
    // sliced position of the ret error signal should be zero If a reshape is done, then the whole
    // tensor would be reset to 0 so it is fine If the shape is preserved and the buffered ret error
    // signal is used, then the buffered ret error signal should not have non-zeros at the positions
    // aforementioned. Therefore, a Fill(0) is not necessary

    // reshape and reset ret signal if input shape changes
    if (inputs.front()->shape() != ret_error_signal_.shape())
    {
      ret_error_signal_.Reshape(inputs.front()->shape());
    }

    // assign the error signal to the correct positions of ret error signal
    if (axes_.empty())
    {
      ret_error_signal_.Slice(index_, axis_).Assign(error_signal);
    }
    else
    {
      ret_error_signal_.Slice(indices_, axes_).Assign(error_signal);
    }
    return {ret_error_signal_};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    std::vector<SizeType> output_shape = inputs.front()->shape();
    // single index
    if (axes_.empty())
    {
      output_shape[axis_] = 1;
    }
    // multi indices
    else
    {
      for (SizeType i : axes_)
      {
        output_shape[i] = 1;
      }
    }
    return output_shape;
  }

  std::vector<SizeType> axes_;
  std::vector<SizeType> indices_;
  SizeType              axis_;
  SizeType              index_;
  TensorType            ret_error_signal_;

  static constexpr OpType OpCode()
  {
    return OpType::OP_SLICE;
  }

  static constexpr char const *DESCRIPTOR = "Slice";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
