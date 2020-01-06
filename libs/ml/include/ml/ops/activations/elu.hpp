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

#include "math/activation_functions/elu.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Elu : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpEluSaveableParams<TensorType>;
  using MyType        = Elu<TensorType>;

  explicit Elu(DataType a)
    : a_(a)
  {}

  explicit Elu(SPType const &sp)
    : Ops<T>(sp)
  {
    a_ = sp.a;
  }

  ~Elu() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    auto sp = std::make_shared<SPType>();
    sp->a   = a_;
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
    assert(inputs.size() == 1);
    fetch::math::Elu((*inputs.front()), a_, output);
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front()->shape() == error_signal.shape());
    TensorType ret{error_signal.shape()};

    auto const zero = DataType{0};
    auto const one  = DataType{1};

    // gradient of elu function is a*e^x where x<0; and 1.0 where x>=0
    auto it  = inputs.front()->cbegin();
    auto rit = ret.begin();
    while (it.is_valid())
    {
      if (*it >= zero)
      {
        // f(x)=x for x>=0
        *rit = one;
      }
      else
      {
        // f(x)=a*e^x
        fetch::math::Exp(*it, *rit);
        fetch::math::Multiply(a_, *rit, *rit);
      }
      ++it;
      ++rit;
    }

    // multiply by error_signal (chain rule)
    fetch::math::Multiply(error_signal, ret, ret);

    return {ret};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_ELU;
  }
  static constexpr char const *DESCRIPTOR = "Elu";

private:
  DataType a_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
