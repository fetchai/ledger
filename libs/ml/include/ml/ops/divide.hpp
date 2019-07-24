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

#include "math/fundamental_operators.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Divide : public fetch::ml::Ops<T>
{
public:
  using ArrayType     = T;
  using SizeType      = typename ArrayType::SizeType;
  using ArrayPtrType  = std::shared_ptr<ArrayType>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using DataType      = typename T::Type;
  using SPType        = SaveableParams;

  Divide() = default;

  explicit Divide(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~Divide() override = default;

  std::shared_ptr<SaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.DESCRIPTOR = DESCRIPTOR;
    return std::make_shared<SPType>(sp);
  }

  /**
   * elementwise division
   * @param inputs  left & right inputs to Divide
   * @return
   */
  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().shape() == output.shape());

    if (inputs.at(1).get().size() > 1)
    {  // array / array
      fetch::math::Divide(inputs.at(0).get(), inputs.at(1).get(), output);
    }
    else
    {  // array / scalar
      fetch::math::Divide(inputs.at(0).get(), *(inputs.at(1).get().cbegin()), output);
    }
  }

  /**
   * f'(a)=(1/b)*err
   * f'(b)=-(a/(b^2))*err
   */
  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    ArrayType return_signal_1(inputs.at(0).get().shape());
    ArrayType return_signal_2(inputs.at(1).get().shape());

    auto a_it   = inputs.at(0).get().cbegin();
    auto b_it   = inputs.at(1).get().cbegin();
    auto err_it = error_signal.cbegin();
    auto r_1_it = return_signal_1.begin();
    auto r_2_it = return_signal_2.begin();
    if (inputs.at(0).get().shape() == inputs.at(1).get().shape())
    {  // array / array same shape
      while (a_it.is_valid())
      {
        *r_1_it = (*err_it) / (*b_it);
        *r_2_it = -((*err_it) * (*a_it)) / ((*b_it) * (*b_it));

        ++a_it;
        ++b_it;
        ++err_it;
        ++r_1_it;
        ++r_2_it;
      }
    }
    else if (inputs.at(1).get().size() == 1)
    {  // array / scalar
      while (a_it.is_valid())
      {
        *r_1_it = (*err_it) / (*b_it);
        *r_2_it += -((*err_it) * (*a_it)) / ((*b_it) * (*b_it));

        ++a_it;
        ++err_it;
        ++r_1_it;
      }
    }
    else
    {  // array / array different shape
       // TODO (#1380) Write backpropagation for array array division of different shapes
      throw std::runtime_error("array array division of different shapes is not yet handled");
    }
    return {return_signal_1, return_signal_2};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front().get().shape();
  }

  static constexpr char const *DESCRIPTOR = "Divide";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
