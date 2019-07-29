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

#include "core/random/lfg.hpp"
#include "math/activation_functions/leaky_relu.hpp"
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "ml/ops/ops.hpp"

#include <cassert>
#include <vector>

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class RandomisedRelu : public fetch::ml::ops::Ops<T>
{
public:
  using ArrayType     = T;
  using DataType      = typename ArrayType::Type;
  using SizeType      = typename ArrayType::SizeType;
  using RNG           = fetch::random::LaggedFibonacciGenerator<>;
  using VecTensorType = typename Ops<T>::VecTensorType;
  using SPType        = RandomisedReluSaveableParams<ArrayType>;

  RandomisedRelu(DataType const lower_bound, DataType const upper_bound,
                 SizeType const &random_seed = 25102015)
    : lower_bound_(lower_bound)
    , upper_bound_(upper_bound)
    , bounds_mean_((upper_bound_ + lower_bound_) / DataType(2))
  {
    rng_.Seed(random_seed);
    UpdateRandomValue();
  }

  explicit RandomisedRelu(SPType const &sp)
  {
    lower_bound_ = sp.lower_bound;
    upper_bound_ = sp.upper_bound;
    bounds_mean_ = ((upper_bound_ + lower_bound_) / DataType(2));
    rng_.Seed(sp.random_seed);
    rng_.SetBuffer(sp.buffer);
    rng_.SetIndex(sp.index);
    //    UpdateRandomValue();
    random_value_ = sp.random_value;
  }

  ~RandomisedRelu() override = default;

  std::shared_ptr<SaveableParamsInterface> GetOpSaveableParams() override
  {
    SPType sp{};
    sp.lower_bound  = lower_bound_;
    sp.upper_bound  = upper_bound_;
    sp.random_seed  = rng_.Seed();
    sp.buffer       = rng_.GetBuffer();
    sp.index        = rng_.GetIndex();
    sp.random_value = random_value_;
    return std::make_shared<SPType>(sp);
  }

  void Forward(VecTensorType const &inputs, ArrayType &output) override
  {
    assert(inputs.size() == 1);
    assert(output.shape() == this->ComputeOutputShape(inputs));

    if (this->is_training_)
    {
      UpdateRandomValue();
    }

    DataType alpha = this->is_training_ ? random_value_ : bounds_mean_;
    fetch::math::LeakyRelu((*inputs.front()), alpha, output);
  }

  std::vector<ArrayType> Backward(VecTensorType const &inputs,
                                  ArrayType const &    error_signal) override
  {
    assert(inputs.size() == 1);
    assert(inputs.front()->shape() == error_signal.shape());
    DataType  zero{0};
    DataType  one{1};
    ArrayType ret{error_signal.shape()};
    ArrayType t{inputs.front()->shape()};

    DataType alpha = this->is_training_ ? random_value_ : bounds_mean_;

    // gradient of randomized-relu function is for x<0 = alpha, x>=0 = 1.0
    this->Forward(inputs, t);

    auto it  = t.cbegin();
    auto rit = ret.begin();
    while (it.is_valid())
    {
      if (*it >= zero)
      {
        // f'(x)=1 for x>=0
        *rit = one;
      }
      else
      {
        // f'(x)=a for x<0
        *rit = alpha;
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

  static constexpr char const *DESCRIPTOR = "RandomisedRelu";

private:
  void UpdateRandomValue()
  {
    random_value_ =
        lower_bound_ + static_cast<DataType>(rng_.AsDouble()) * (upper_bound_ - lower_bound_);
  }

  DataType random_value_;
  DataType lower_bound_;
  DataType upper_bound_;
  DataType bounds_mean_;
  RNG      rng_;
};

}  // namespace ops
}  // namespace ml
}  // namespace fetch
