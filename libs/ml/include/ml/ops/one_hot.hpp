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

#include "math/one_hot.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <utility>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class OneHot : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = typename TensorType::SizeType;
  using DataType      = typename TensorType::Type;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpOneHotSaveableParams<T>;
  using MyType        = OneHot<TensorType>;

  /**
   * One hot function based on tf.one_hot
   * @param depth number of classes
   * @param axis
   * @param on_value TRUE value
   * @param off_value FALSE value
   */
  explicit OneHot(SizeType depth, SizeType axis = 0, DataType on_value = DataType{1},
                  DataType off_value = DataType{0})
    : depth_(depth)
    , axis_(axis)
    , on_value_(on_value)
    , off_value_(off_value)
  {}

  explicit OneHot(SPType const &sp)
    : Ops<T>(sp)
  {
    depth_     = sp.depth;
    axis_      = sp.axis;
    on_value_  = sp.on_value;
    off_value_ = sp.off_value;
  }

  ~OneHot() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.depth     = depth_;
    sp.axis      = axis_;
    sp.on_value  = on_value_;
    sp.off_value = off_value_;

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
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::OneHot(output, *(inputs.at(0)), depth_, axis_, on_value_, off_value_);
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(error_signal);
    assert(inputs.size() == 1);
    assert(error_signal.shape() == this->ComputeOutputShape(inputs));

    // No derivative defined for OneHotOp
    return {TensorType{inputs.at(0)->shape()}};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    assert(inputs.size() == 1);

    std::vector<SizeType> shape = inputs.at(0)->shape();

    if (axis_ == shape.size())
    {
      shape.emplace_back(depth_);
    }
    else
    {
      shape.insert(shape.begin() + static_cast<math::PtrDiffType>(axis_), depth_);
    }

    return shape;
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_ONE_HOT;
  }
  static constexpr char const *DESCRIPTOR = "OneHot";

private:
  SizeType depth_;
  SizeType axis_;
  DataType on_value_  = DataType{1};
  DataType off_value_ = DataType{0};
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
