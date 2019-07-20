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

#include <cassert>
#include <memory>
#include <vector>

#include "math/activation_functions/sigmoid.hpp"
#include "math/activation_functions/softmax.hpp"
#include "math/fundamental_operators.hpp"
#include "math/metrics/cross_entropy.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class CrossEntropyLoss : public Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;

  CrossEntropyLoss()          = default;
  virtual ~CrossEntropyLoss() = default;

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 2);
    assert(inputs.at(0).get().size() == inputs.at(1).get().size());

    output(0, 0) = fetch::math::CrossEntropyLoss(inputs.at(0).get(), inputs.at(1).get());
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    FETCH_UNUSED(error_signal);

    assert(inputs.size() == 2);
    assert(inputs.at(0).get().size() == inputs.at(1).get().size());
    assert(inputs.at(0).get().shape().size() == 2);

    ArrayType ret({inputs.at(0).get().shape()});
    if (inputs.at(0).get().shape().at(0) == 1)  // not one-hot
    {
      // (Sigmoid(x)-y)*x
      auto     a_it = inputs.at(0).get().cbegin();
      auto     b_it = inputs.at(1).get().cbegin();
      auto     r_it = ret.begin();
      DataType zero{0};
      DataType one{1};

      while (a_it.is_valid())
      {
        // Sigmoid(x)
        if (*a_it >= zero)
        {
          fetch::math::Exp(-(*a_it), *r_it);
          *r_it = one / (one + (*r_it));
        }
        else
        {
          fetch::math::Exp(*a_it, *r_it);
          *r_it = *r_it / (*r_it + one);
        }

        // (Sigmoid(x)-y)*x
        *r_it = (*r_it - (*b_it)) * (*a_it);

        ++a_it;
        ++b_it;
        ++r_it;
      }
    }
    else if (inputs.at(0).get().shape().size())  // one-hot
    {
      fetch::math::Softmax(inputs.at(0).get(), ret, 1);

      auto b_it = inputs.at(1).get().cbegin();
      auto r_it = ret.begin();
      while (b_it.is_valid())
      {
        *r_it = -(*b_it) / (*r_it);
        ++b_it;
        ++r_it;
      }
    }

    return {ret, ret};
  }

  std::vector<typename T::SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    (void)inputs;
    return {1, 1};
  }

  static constexpr char const *DESCRIPTOR = "CrossEntropyLoss";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
