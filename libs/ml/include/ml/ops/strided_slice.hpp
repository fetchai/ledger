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
class StridedSlice : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  // using ConstSliceType = typename TensorType::ConstSliceType;
  using SPType = OpStridedSliceSaveableParams<T>;
  using MyType = StridedSlice<TensorType>;

  explicit StridedSlice(std::vector<SizeType> const &begins, std::vector<SizeType> const &ends,
                        std::vector<SizeType> const &strides)
    : begins_(std::move(begins))
    , ends_(std::move(ends))
    , strides_(std::move(strides))

  {}

  explicit StridedSlice(SPType const &sp)
    : Ops<T>(sp)
  {
    begins_  = sp.begins;
    ends_    = sp.ends;
    strides_ = sp.strides;
  }

  ~StridedSlice() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp = std::make_shared<SPType>();

    sp->begins  = begins_;
    sp->ends    = ends_;
    sp->strides = strides_;

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

    FETCH_UNUSED(inputs);
    FETCH_UNUSED(output);

    /* // Copying is necessary because ranged StridedSlice is non-const
     TensorType input = inputs.front()->Copy();
     output.Assign(input.Slice(start_end_StridedSlice_, axis_));*/
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    FETCH_UNUSED(error_signal);
    /*
    FETCH_UNUSED(inputs);
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    // N.B. At this position of the code, we have to make sure that every position other than the
    // StridedSliced position of the ret error signal should be zero If a reshape is done, then the
    whole
    // tensor would be reset to 0 so it is fine If the shape is preserved and the buffered ret error
    // signal is used, then the buffered ret error signal should not have non-zeros at the positions
    // aforementioned. Therefore, a Fill(0) is not necessary

    // reshape and reset ret signal if input shape changes
    if (inputs.front()->shape() != ret_error_signal_.shape())
    {
      ret_error_signal_.Reshape(inputs.front()->shape());
    }

    // assign the error signal to the correct positions of ret error signal
    switch (StridedSlice_type_)
    {
    case StridedSliceType::SINGLE_AXIS:
    {
      ret_error_signal_.StridedSlice(index_, axis_).Assign(error_signal);
      break;
    }
    case StridedSliceType::MULTI_AXIS:
    {
      ret_error_signal_.StridedSlice(indices_, axes_).Assign(error_signal);
      break;
    }
    case StridedSliceType::RANGED:
    {
      ret_error_signal_.StridedSlice(start_end_StridedSlice_, axis_).Assign(error_signal);
      break;
    }
    }

    return {ret_error_signal_};*/

    return {};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {

    std::vector<SizeType> output_shape = inputs.front()->shape();

    for (SizeType i{0}; i < output_shape.size(); i++)
    {
    }

    return output_shape;
  }

  std::vector<SizeType> begins_;
  std::vector<SizeType> ends_;
  std::vector<SizeType> strides_;
  TensorType            ret_error_signal_;

  static constexpr OpType OpCode()
  {
    return OpType::OP_STRIDED_SLICE;
  }

  static constexpr char const *DESCRIPTOR = "StridedSlice";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
