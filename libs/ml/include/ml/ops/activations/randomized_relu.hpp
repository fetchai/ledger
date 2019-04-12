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
#include "math/fundamental_operators.hpp"
#include "math/matrix_operations.hpp"
#include "math/ml/activation_functions/leaky_relu.hpp"
#include "ml/ops/ops.hpp"

namespace fetch {
namespace ml {
namespace ops {

template <class T>
class RandomizedRelu : public fetch::ml::ElementWiseOps<T>
{
public:
  using ArrayType    = T;
  using DataType     = typename ArrayType::Type;
  using ArrayPtrType = std::shared_ptr<ArrayType>;
  using SizeType     = typename ArrayType::SizeType;
  using RNG          = fetch::random::LaggedFibonacciGenerator<>;

  RandomizedRelu(DataType const lower_bound, DataType const upper_bound,
                 SizeType const &random_seed = 25102015)
    : lower_bound_(lower_bound)
    , upper_bound_(upper_bound)
    , bounds_mean_((upper_bound_ + lower_bound_) / DataType(2))
  {
    rng_.Seed(random_seed);
    UpdateRandomValue();
  }

  virtual ~RandomizedRelu() = default;

  virtual ArrayType Forward(std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
                            ArrayType &                                                 output)
  {
    assert(inputs.size() == 1);

    if (this->is_training_)
    {
      UpdateRandomValue();
    }

    DataType tmp_alpha = this->is_training_ ? random_value_ : bounds_mean_;
    fetch::math::LeakyRelu(inputs.front().get(), tmp_alpha, output);

    return output;
  }

  virtual std::vector<ArrayType> Backward(
      std::vector<std::reference_wrapper<ArrayType const>> const &inputs,
      ArrayType const &                                           errorSignal)
  {
    assert(inputs.size() == 1);
    assert(inputs.front().get().shape() == errorSignal.shape());
    ArrayType returnSignal{errorSignal.shape()};

    DataType tmp_alpha = this->is_training_ ? random_value_ : bounds_mean_;

    // gradient of randomized-relu function is for x<0 = alpha, x>=0 = 1.0
    typename ArrayType::SizeType idx(0);
    for (auto const &val : inputs.at(0).get())
    {
      if (val >= DataType(0))
      {
        returnSignal.Set(idx, DataType(1));
      }
      else
      {
        returnSignal.Set(idx, tmp_alpha);
      }
      ++idx;
    }

    // multiply by errorSignal (chain rule)
    fetch::math::Multiply(errorSignal, returnSignal, returnSignal);

    return {returnSignal};
  }

  static constexpr char const *DESCRIPTOR = "RandomizedRelu";

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
