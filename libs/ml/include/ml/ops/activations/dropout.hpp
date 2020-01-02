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

#include "core/macros.hpp"
#include "core/random/lfg.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class Dropout : public fetch::ml::ops::Ops<T>
{
public:
  using TensorType    = T;
  using DataType      = typename TensorType::Type;
  using SizeType      = fetch::math::SizeType;
  using RNG           = fetch::random::LaggedFibonacciGenerator<>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = OpDropoutSaveableParams<TensorType>;
  using MyType        = Dropout<TensorType>;

  explicit Dropout(DataType const probability, SizeType const &random_seed = 25102015)
    : probability_(probability)
  {
    if (probability < DataType{0} || probability > DataType{1})
    {
      std::stringstream ss;
      ss << probability;
      throw std::runtime_error("Dropout probability " + ss.str() +
                               " is out of allowed range [0..1]");
    }
    rng_.Seed(random_seed);
    drop_values_ = TensorType{0};
  }

  explicit Dropout(SPType const &sp)
    : Ops<T>(sp)
  {
    probability_ = sp.probability;
    rng_.Seed(sp.random_seed);
    rng_.SetBuffer(sp.buffer);
    rng_.SetIndex(sp.index);
  }

  ~Dropout() override = default;

  std::shared_ptr<OpsSaveableParams> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.probability = probability_;
    sp.random_seed = rng_.Seed();
    sp.buffer      = rng_.GetBuffer();
    sp.index       = rng_.GetIndex();
    return std::make_shared<SPType>(sp);
  }

  std::shared_ptr<fetch::ml::ops::Ops<TensorType>> MakeSharedCopy(
      std::shared_ptr<fetch::ml::ops::Ops<TensorType>> me) override
  {
    assert(me.get() == this);

    return me;
  }

  void Forward(VecTensorType const &inputs, TensorType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    if (!this->is_training_)
    {
      output.Copy((*inputs.front()));
    }
    else
    {
      if (drop_values_.shape() != output.shape())
      {
        drop_values_ = TensorType(output.shape());
      }

      auto out_it = output.begin();
      auto in_it  = inputs.front()->cbegin();
      auto it     = drop_values_.begin();
      while (it.is_valid())
      {
        if (rng_.AsType<DataType>() <= probability_)
        {
          *it     = DataType{1} / probability_;
          *out_it = (*it) * (*in_it);
        }
        else
        {
          *it     = DataType{0};
          *out_it = DataType{0};
        }
        ++it;
        ++in_it;
        ++out_it;
      }
    }
  }

  std::vector<TensorType> Backward(VecTensorType const &inputs,
                                   TensorType const &   error_signal) override
  {
    FETCH_UNUSED(inputs);
    assert(inputs.size() == 1);
    assert(error_signal.shape() == inputs.front()->shape());
    assert(drop_values_.shape() == inputs.front()->shape());
    assert(this->is_training_);

    TensorType return_signal{error_signal.shape()};

    // gradient of dropout is 1.0/keep_prob for enabled neurons and 0.0 for disabled
    // multiply by error_signal (chain rule)

    fetch::math::Multiply(error_signal, drop_values_, return_signal);

    return {return_signal};
  }

  std::vector<SizeType> ComputeOutputShape(VecTensorType const &inputs) const override
  {
    return inputs.front()->shape();
  }

  static constexpr OpType OpCode()
  {
    return OpType::OP_DROPOUT;
  }
  static constexpr char const *DESCRIPTOR = "Dropout";

private:
  TensorType drop_values_;
  DataType   probability_;
  RNG        rng_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
