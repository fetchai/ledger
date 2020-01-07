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
class StridedSlice : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = fetch::math::SizeType;
  using SizeVector    = fetch::math::SizeVector;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpStridedSliceSaveableParams<T>;
  using MyType        = StridedSlice<TensorType>;

  explicit StridedSlice(SizeVector const &begins, SizeVector const &ends,
                        SizeVector const &strides = {})
    : begins_(begins)
    , ends_(ends)
    , strides_(strides)
  {
    assert(begins.size() == ends.size());

    // Correction to match tf.StridedSlice
    for (SizeType i{0}; i < ends_.size(); i++)
    {
      ends_[i] = ends[i] + 1;
    }

    if (strides.empty())
    {
      strides_ = begins;
      for (SizeType i{0}; i < strides.size(); i++)
      {
        strides_.at(i) = 1;
      }
    }
  }

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

  /**
   * Forward pass is done by assigning values in given ranges with stride size step for every
   * dimmension from larger input tensor to smaller output tensor.
   * @param inputs
   * @param output
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    auto slice = inputs.at(0)->Slice(begins_, ends_, strides_);
    output.Assign(slice);
  }

  /**
   * Backward pass is done by assigning smaller error signal tensor to larger return signal tensor
   * @param inputs
   * @param error_signal
   * @return
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    TensorType ret_error_signal_{inputs.at(0)->shape()};

    auto slice = ret_error_signal_.Slice(begins_, ends_, strides_);
    slice.Assign(error_signal);

    return {ret_error_signal_};
  }

  SizeVector ComputeOutputShape(VecTensorType const &inputs) const override
  {

    SizeVector output_shape = inputs.front()->shape();

    // Calculate number of stride size steps from specified begin to specified end for each
    // dimension
    for (SizeType i{0}; i < begins_.size(); i++)
    {
      assert(strides_.at(i) != 0);
      assert(begins_.at(i) <= ends_.at(i));
      output_shape.at(i) = ((ends_.at(i) - begins_.at(i) - 1) / strides_.at(i)) + 1;
    }

    return output_shape;
  }

  SizeVector begins_;
  SizeVector ends_;
  SizeVector strides_;

  static constexpr OpType OpCode()
  {
    return OpType::OP_STRIDED_SLICE;
  }

  static constexpr char const *DESCRIPTOR = "StridedSlice";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
