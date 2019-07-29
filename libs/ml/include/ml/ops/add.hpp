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
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = AddSaveableParams<T>;

  Add() = default;

  explicit Add(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Add() override = default;

  std::shared_ptr<SaveableParamsInterface> GetOpSaveableParams() override
  {
    return std::make_shared<SPType>();
  }

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

    // Test if input is broadcastable by batch dimension
    assert(inputs.at(1)->shape().at(inputs.at(1)->shape().size() - 1) == 1);
    for (SizeType i{0}; i < inputs.at(0)->shape().size() - 1; i++)
    {
      assert(inputs.at(0)->shape().at(i) == inputs.at(1)->shape().at(i));
    }

    if (inputs.at(0)->shape() == inputs.at(1)->shape())
    {
      return {error_signal, error_signal};
    }
    else
    {
      SizeType batch_dimension = inputs.at(0)->shape().size() - 1;
      return {error_signal, fetch::math::ReduceSum(error_signal, batch_dimension)};
    }
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.at(0)->shape();
  }

  static constexpr char const *DESCRIPTOR = "Add";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
