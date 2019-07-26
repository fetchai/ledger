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

#include "core/assert.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Maximum : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = SaveableParams;

  Maximum() = default;

  explicit Maximum(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Maximum() override = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<SPType>(sp);
  }

  /**
   * elementwise maximum
   * @param inputs  left & right inputs to get maximum
   * @return
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::Maximum((*inputs.at(0)), (*inputs.at(1)), output);
  }

  /**
   * elementwise maximum gradient is:
   * f'(input0)=if(input0>input1)=error_signal
   * f'(input1)=if(input0<=input1)=error_signal
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());
    assert(error_signal.size() == inputs.at(1)->size());

    ArrayType return_signal_1(inputs.at(0)->shape());
    ArrayType return_signal_2(inputs.at(1)->shape());

    auto a_it   = inputs.at(0)->cbegin();
    auto b_it   = inputs.at(1)->cbegin();
    auto err_it = error_signal.cbegin();
    auto r_1_it = return_signal_1.begin();
    auto r_2_it = return_signal_2.begin();
    while (a_it.is_valid())
    {
      if ((*a_it) > (*b_it))
      {
        *r_1_it = *err_it;
      }
      else
      {
        *r_2_it = *err_it;
      }

      ++a_it;
      ++b_it;
      ++err_it;
      ++r_1_it;
      ++r_2_it;
    }

    return {return_signal_1, return_signal_2};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr char const *DESCRIPTOR = "Maximum";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
