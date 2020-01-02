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
#include "ml/saveparams/saveable_params.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Add : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpAddSaveableParams<T>;
  using MyType        = Add<TensorType>;

  Add() = default;

  explicit Add(SPType const &sp)
    : Ops<T>(sp)
  {
    axes_ = sp.axes;
  }

  ~Add() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto ret  = std::make_shared<SPType>();
    ret->axes = axes_;
    return ret;
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

    return copyshare;
  }

  // for inputs to the add layer, if broadcasting is required, make sure the first input is the one
  // with the complete shape
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 2);
    assert(output.shape() == this->ComputeOutputShape(inputs));
    fetch::math::Add((*inputs.at(0)), (*inputs.at(1)), output);
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->shape().size() == inputs.at(1)->shape().size());
    assert(inputs.at(0)->shape() == error_signal.shape());
    assert(error_signal.shape() == ComputeOutputShape(inputs));

    if (inputs.at(0)->shape() == inputs.at(1)->shape())
    {
      // Non-broadcast Add
      return {error_signal, error_signal};
    }

    // Broadcast Add
    UpdateAxes(inputs);
    return {error_signal, fetch::math::ReduceSum(error_signal, axes_)};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.at(0)->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_ADD;
  }

  static constexpr char const *DESCRIPTOR = "Add";

private:
  std::vector<SizeType> axes_;

  void UpdateAxes(VecTensorType const &inputs)
  {
    bool axes_changed = false;

    // Check if axes were changed
    SizeType cnt = 0;
    for (SizeType i{0}; i < inputs.at(0)->shape().size(); i++)
    {
      if (inputs.at(0)->shape().at(i) != inputs.at(1)->shape().at(i))
      {
        if (cnt >= axes_.size() || axes_.at(cnt) != i)
        {
          axes_changed = true;
          break;
        }
        cnt++;
      }
    }

    if (axes_.empty())
    {
      axes_changed = true;
    }

    // Update axes if necessary
    if (axes_changed)
    {
      axes_.clear();
      // Get axes
      for (SizeType i{0}; i < inputs.at(0)->shape().size(); i++)
      {
        if (inputs.at(0)->shape().at(i) != inputs.at(1)->shape().at(i))
        {
          axes_.emplace_back(i);
        }
      }
    }
  }
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
