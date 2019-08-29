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

#include "math/activation_functions/sigmoid.hpp"
#include "math/activation_functions/softmax.hpp"
#include "math/fundamental_operators.hpp"
#include "math/metrics/cross_entropy.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <memory>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class CrossEntropyLoss : public Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = typename TensorType::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpCrossEntropyLossSaveableParams<TensorType>;
  using MyType        = CrossEntropyLoss<TensorType>;

  CrossEntropyLoss() = default;

  explicit CrossEntropyLoss(SPType const &sp)
    : Ops<T>(sp)
  {}

  ~CrossEntropyLoss() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp = std::make_shared<SPType>();
    return sp;
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
    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());

    output(0, 0) = fetch::math::CrossEntropyLoss((*inputs.at(0)), (*inputs.at(1)));
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(error_signal);

    assert(inputs.size() == 2);
    assert(inputs.at(0)->size() == inputs.at(1)->size());
    assert(inputs.at(0)->shape().size() == 2);

    TensorType ret({inputs.at(0)->shape()});
    if (inputs.at(0)->shape().at(0) == 1)  // not one-hot
    {
      // (Sigmoid(x)-y)*x
      auto     a_it = inputs.at(0)->cbegin();
      auto     b_it = inputs.at(1)->cbegin();
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
    else if (inputs.at(0)->shape().size())  // one-hot
    {
      fetch::math::Softmax((*inputs.at(0)), ret, 0);

      auto b_it = inputs.at(1)->cbegin();
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

  static constexpr OpType OpCode()
  {
    return OpType::OP_CROSS_ENTROPY_LOSS;
  }
  static constexpr char const *DESCRIPTOR = "CrossEntropyLoss";
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
