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
  using SizeType       = fetch::math::SizeType;
  using ArrayPtrType   = std::shared_ptr<TensorType>;
  using VecTensorType  = typename Ops<T>::VecTensorType;
  using ConstSliceType = typename TensorType::ConstSliceType;
  using SPType         = OpSliceSaveableParams<T>;
  using MyType         = Slice<TensorType>;

  explicit Slice(std::vector<SizeType> indices, std::vector<SizeType> axes)
  {
    indices_    = indices;
    axes_       = axes;
    slice_type_ = SliceType::MULTI_AXIS;
  }
  explicit Slice(SizeType index, SizeType axis)
  {
    index_      = index;
    axis_       = axis;
    slice_type_ = SliceType::SINGLE_AXIS;
  }

  explicit Slice(std::pair<SizeType, SizeType> start_end_slice, SizeType axis)
  {
    start_end_slice_ = start_end_slice;
    axis_            = axis;
    slice_type_      = SliceType::RANGED;
  }

  explicit Slice(SPType const &sp)
    : Ops<T>(sp)
  {
    indices_         = sp.indices;
    axes_            = sp.axes;
    index_           = sp.index;
    axis_            = sp.axis;
    start_end_slice_ = sp.start_end_slice;
    slice_type_      = SliceType(sp.slice_type);
  }

  ~Slice() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp = std::make_shared<SPType>();

    sp->indices         = indices_;
    sp->axes            = axes_;
    sp->index           = index_;
    sp->axis            = axis_;
    sp->start_end_slice = start_end_slice_;
    sp->slice_type      = static_cast<uint8_t>(slice_type_);

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

    switch (slice_type_)
    {
    case SliceType::SINGLE_AXIS:
    {
      output.Assign(inputs.front()->Slice(index_, axis_));
      break;
    }
    case SliceType::MULTI_AXIS:
    {
      output.Assign(inputs.front()->Slice(indices_, axes_));
      break;
    }
    case SliceType::RANGED:
    {
      // Copying is necessary because ranged slice is non-const
      TensorType input = inputs.front()->Copy();
      output.Assign(input.Slice(start_end_slice_, axis_));
      break;
    }
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
    switch (slice_type_)
    {
    case SliceType::SINGLE_AXIS:
    {
      ret_error_signal_.Slice(index_, axis_).Assign(error_signal);
      break;
    }
    case SliceType::MULTI_AXIS:
    {
      ret_error_signal_.Slice(indices_, axes_).Assign(error_signal);
      break;
    }
    case SliceType::RANGED:
    {
      ret_error_signal_.Slice(start_end_slice_, axis_).Assign(error_signal);
      break;
    }
    }

    return {ret_error_signal_};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    std::vector<SizeType> output_shape = inputs.front()->shape();

    switch (slice_type_)
    {
    case SliceType::SINGLE_AXIS:
    {
      output_shape[axis_] = 1;
      break;
    }
    case SliceType::MULTI_AXIS:
    {
      for (SizeType i : axes_)
      {
        output_shape[i] = 1;
      }
      break;
    }
    case SliceType::RANGED:
    {
      output_shape[axis_] = start_end_slice_.second - start_end_slice_.first;
      break;
    }
    }

    return output_shape;
  }

  std::pair<SizeType, SizeType> start_end_slice_;
  std::vector<SizeType>         axes_;
  std::vector<SizeType>         indices_;
  SizeType                      axis_;
  SizeType                      index_;
  TensorType                    ret_error_signal_;

  SliceType slice_type_;

  static constexpr OpType OpCode()
  {
    return OpType::OP_SLICE;
  }

  static constexpr char const *DESCRIPTOR = "Slice";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
