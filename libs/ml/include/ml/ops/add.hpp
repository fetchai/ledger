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
class Add : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  Add()           = default;
  ~Add() override = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 2);
    assert(output.shape() == this->ComputeOutputShape(inputs));
    fetch::math::Add((*inputs.at(0)), (*inputs.at(1)), output);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
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
    else
    {
      // Broadcast Add
      bool axes_changed = false;

      // Check if axes were changed
      SizeType cnt = 0;
      for (SizeType i{0}; i < inputs.at(0)->shape().size(); i++)
      {
        if (inputs.at(0)->shape().at(i) != inputs.at(1)->shape().at(i))
        {
          if (cnt >= this->axes_.size() || this->axes_.at(cnt) != i)
          {
            axes_changed = true;
            break;
          }
          cnt++;
        }
      }

      if (this->axes_.size() == 0)
      {
        axes_changed = true;
      }

      // Update axes if necessary
      if (axes_changed)
      {
        this->axes_.clear();
        // Get axes
        for (SizeType i{0}; i < inputs.at(0)->shape().size(); i++)
        {
          if (inputs.at(0)->shape().at(i) != inputs.at(1)->shape().at(i))
          {
            this->axes_.emplace_back(i);
          }
        }
      }

      return {error_signal, fetch::math::ReduceSum(error_signal, axes_)};
    }
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.at(0)->shape();
  }

  std::vector<SizeType> axes_;

  static constexpr char const *DESCRIPTOR = "Add";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
