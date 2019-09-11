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
#include "ml/saveparams/saveable_params.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Abs : public Ops<T>
{
public:
  using TensorType    = T;
  using SizeType      = typename TensorType::SizeType;
  using ArrayPtrType  = std::shared_ptr<TensorType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpAbsSaveableParams<T>;
  using MyType        = Abs<TensorType>;

  Abs() = default;

  explicit Abs(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Abs() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    return std::make_shared<SPType>();
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    FETCH_UNUSED(me);
    assert(me.get() == this);

    auto copyshare = std::make_shared<MyType>(*this);  // calls default copy constructor of MyType

    return copyshare;
  }

  /**
   * elementwise absolute value
   * @param inputs - one input for elementwise abs
   * @return
   */
  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(inputs.at(0)->shape() == output.shape());
    assert(output.shape() == this->ComputeOutputShape(inputs));

    fetch::math::Abs((*inputs.at(0)), output);
  }

  /**
   * elementwise absolute value gradient is:
   * f'(input0)=sign(input0)*error_signal
   */
  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(error_signal.size() == inputs.at(0)->size());

    TensorType return_signal(inputs.at(0)->shape());

    auto a_it   = inputs.at(0)->cbegin();
    auto err_it = error_signal.cbegin();
    auto r_it   = return_signal.begin();
    while (a_it.is_valid())
    {
      if (*a_it > 0)
      {
        *r_it = *err_it;
      }
      else
      {
        *r_it = -*err_it;
      }

      ++a_it;
      ++err_it;
      ++r_it;
    }

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_ABS;
  }

  static constexpr char const *DESCRIPTOR = "Abs";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
